#include <desaster/Server.h>
#include <desaster/Module.h>
#include <desaster/Queue.h>
#include <desaster/config.h>

#include <desaster/NetworkAdapter.h>
#include <desaster/ClusterAdapter.h>

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
#include <getopt.h>

Server::Server(ev::loop_ref loop) :
	Logging("core"),
	loop_(loop),
	port_(2691),
	bindAddress_("0.0.0.0"),
	brdAddress_("255.255.255.255"),
	modules_(),
	queues_(),
	terminateSignal_(loop_),
	interruptSignal_(loop_)
{
	terminateSignal_.set<Server, &Server::terminateSignal>(this);
	terminateSignal_.set(SIGTERM);
	terminateSignal_.start();
	loop_.unref();

	interruptSignal_.set<Server, &Server::interruptSignal>(this);
	interruptSignal_.set(SIGINT);
	interruptSignal_.start();
	loop_.unref();

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

bool Server::setup(int argc, char* argv[])
{
	static const struct option long_options[] = {
		{ "help", no_argument, nullptr, 'h' },
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
	std::string clusterGroup;
	int clusterPort = -1;
	bool standalone = false;

	for (bool done = false; !done;) {
		int long_index = 0;

		switch (getopt_long(argc, argv, "?ha:p:A:B:P:G:S", long_options, &long_index)) {
			case '?':
			case 'h':
				printHelp();
				return false;
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
				clusterPort = atoi(optarg);
				break;
			case 'G':
				clusterGroup = optarg;
				break;
			case 'S':
				standalone = true;
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

	if (!module<NetworkAdapter>()->configure(port, bindAddress))
		return false;

	for (auto module: modules_)
		if (!module->start())
			return false;

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
		" usage: desaster [-a BIND_ADDR] [-b BROADCAST_ADDR] [-p PORT] [-k GROUP_KEY]\n"
		"\n"
		" -?, -h, --help                 prints this help\n"
		" -p, --port=NUMBER              port number for receiving/sending packets [%d]\n"
		" -a, --bind-address=IPADDR      local IP address to bind to [%s]\n"
		" -b, --broadcast-address=IPADDR remote IP/multicast/broadcast address to announce to [%s]\n"
		" -k, --key=GROUP_KEY            cluster-group shared key [%s]\n"
		" -s, --standalone               do not broadcast for peering with cluster\n"
		"\n",
		PACKAGE_VERSION,
		homepageUrl,
		module<NetworkAdapter>()->port(),
		module<NetworkAdapter>()->bindAddress().c_str(),
		module<ClusterAdapter>()->brdAddress().c_str(),
		module<ClusterAdapter>()->groupName().c_str()
	);
}
