#include <desaster/Server.h>
#include <desaster/Module.h>
#include <desaster/Queue.h>
#include <desaster/config.h>

#include <desaster/NetworkAdapter.h>
#include <desaster/ClusterAdapter.h>

#include "sd-daemon.h"

#include <iostream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

/** concats a path with a filename and optionally inserts a path seperator if path 
 *  doesn't contain a trailing path seperator. */
static inline std::string pathcat(const std::string& path, const std::string& filename)
{
	if (!path.empty() && path[path.size() - 1] != '/')
		return path + "/" + filename;
	else
		return path + filename;
}

Server::Server(ev::loop_ref loop) :
	Logging("core"),
	loop_(loop),
	logFileName_(pathcat(DESASTER_LOGDIR, "desaster.log")),
	modules_(),
	queues_(),
	terminateSignal_(loop_),
	interruptSignal_(loop_)
{
	registerModule(new NetworkAdapter(this));
	registerModule(new ClusterAdapter(this));
}

Server::~Server()
{
	stop();

	while (!modules_.empty())
		delete unregisterModule(*modules_.begin());
}

Module* Server::registerModule(Module* m)
{
	modules_.push_back(m);
	return m;
}

Module* Server::unregisterModule(Module* m)
{
	auto i = std::find(modules_.begin(), modules_.end(), m);
	if (i != modules_.end())
		modules_.erase(i);

	return m;
}

bool Server::start(int argc, char* argv[])
{
	static const struct option long_options[] = {
		{ "help", no_argument, nullptr, 'h' },
		{ "daemonize", required_argument, nullptr, 'd' },
		{ "log", required_argument, nullptr, 'l' },
		{ "port", required_argument, nullptr, 'p' },
		{ "bind-address", required_argument, nullptr, 'a' },
		{ "cluster-broadcast-address", required_argument, nullptr, 'B' },
		{ "cluster-address", required_argument, nullptr, 'A' },
		{ "cluster-port", required_argument, nullptr, 'P' },
		{ "cluster-group", required_argument, nullptr, 'G' },
		{ "standalone", no_argument, nullptr, 'S' },
		{ 0, 0, 0, 0 }
	};

	// network adapter
	std::string bindAddress = module<NetworkAdapter>()->bindAddress();
	int port = module<NetworkAdapter>()->port();

	// cluster adapter
	std::string clusterAddress;
	std::string clusterBrdAddress;
	bool daemonize = false;

	for (bool done = false; !done;) {
		int long_index = 0;

		switch (getopt_long(argc, argv, "?hdl:a:p:A:B:P:G:S", long_options, &long_index)) {
			case '?':
			case 'h':
				printHelp();
				return false;
			case 'd':
				daemonize = true;
				break;
			case 'l':
				logFileName_ = optarg;
				break;
			case 'a':
				bindAddress = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'A':
				clusterAddress = optarg;
				break;
			case 'B':
				clusterBrdAddress = optarg;
				break;
			case 'P':
				module<ClusterAdapter>()->setPort(atoi(optarg));
				break;
			case 'G':
				module<ClusterAdapter>()->setGroupName(optarg);
				break;
			case 'S':
				//module<ClusterAdapter>()->setMode(ClusterMode::Standalone);
				break;
			case 0:
				// long opt with val != NULL
				break;
			case -1: // EOF
				done = true;
				break;
			default:
				return false;
		}
	}

	if (daemonize && (getppid() != 1 || !sd_booted())) {
		// redirect stdio only when daemonizing and not controlled by systemd
		int nullFd = open("/dev/null", O_RDONLY);
		if (nullFd < 0) {
			error("Could not open /dev/null: %s", strerror(errno));
			return false;
		}

		int logFd = open(logFileName_.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
		if (logFd < 0) {
			error("Could not open %s: %s", logFileName_.c_str(), strerror(errno));
			close(nullFd);
			return false;
		}

		close(STDIN_FILENO);
		dup2(nullFd, STDIN_FILENO);

		close(STDOUT_FILENO);
		dup2(logFd, STDOUT_FILENO);

		close(STDERR_FILENO);
		dup2(logFd, STDOUT_FILENO);
	}

	if (!module<NetworkAdapter>()->configure(port, bindAddress))
		return false;

	for (auto module: modules_)
		if (!module->start())
			return false;

	terminateSignal_.set<Server, &Server::terminateSignal>(this);
	terminateSignal_.set(SIGTERM);
	terminateSignal_.start();
	loop_.unref();

	interruptSignal_.set<Server, &Server::interruptSignal>(this);
	interruptSignal_.set(SIGINT);
	interruptSignal_.start();
	loop_.unref();

	return true;
}

void Server::stop()
{
	for (auto module: modules_)
		module->stop();

	if (terminateSignal_.is_active()) {
		loop_.ref();
		terminateSignal_.stop();
	}

	if (interruptSignal_.is_active()) {
		loop_.ref();
		interruptSignal_.stop();
	}
}

void Server::terminateSignal(ev::sig& sig, int revents)
{
	notice("Terminate signal received. Stopping");
	stop();
}

void Server::interruptSignal(ev::sig& sig, int revents)
{
	notice("Interrupt signal received. Stopping");
	stop();
}

void Server::printHelp()
{
	const char* homepageUrl = "http://github.com/trapni/desaster/";

	std::printf(
		"desaster, Job Queueing Manager, version %s [%s]\n"
		"Copyright (c) 2012 by Christian Parpart <trapni@gentoo.org>\n"
		"Licensed under GPL-3 [http://gplv3.fsf.org/]\n"
		"\n"
		" usage: desaster [options ...]\n"
		"\n"
		" -?, -h, --help                         prints this help\n"
		" -d, --daemonize                        fork into background\n"
		" -l, --log=PATH                         path to log file [%s]\n"
		"\n"
		" -a, --bind-address=IPADDR              local IP address to bind to [%s]\n"
		" -p, --port=NUMBER                      port number for receiving/sending packets [%d]\n"
		"\n"
		" -K, --cluster-group=GROUP_KEY          cluster-group shared key [%s]\n"
		" -K, --cluster-bind-address=IPADDR      cluster syncronization bind address [%s]\n"
		" -B, --cluster-broadcast-address=IPADDR remote IP/multicast/broadcast address to\n"
		"                                        announce to [%s]\n"
		" -P, --cluster-port=NUMBER              clsuter tcp/udp port number [%d]\n"
		" -S, --standalone                       do not broadcast for peering with cluster\n"
		"\n",
		PACKAGE_VERSION,
		homepageUrl,
		logFileName_.c_str(),
		module<NetworkAdapter>()->bindAddress().c_str(),
		module<NetworkAdapter>()->port(),
		module<ClusterAdapter>()->groupName().c_str(),
		module<ClusterAdapter>()->bindAddress().c_str(),
		module<ClusterAdapter>()->brdAddress().c_str(),
		module<ClusterAdapter>()->port()
	);
}
