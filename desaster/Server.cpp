#include <desaster/Server.h>
#include <desaster/Connection.h>
#include <desaster/Module.h>
#include <desaster/Queue.h>
#include <desaster/config.h>

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
	state_(State::Stopped),
	schedulerRole_(SchedulerRole::Slave),
	clusterGroup_("default"),
	loop_(loop),
	peeringTimer_(loop_),
	peeringAttemptTimeout_(5), // seconds
	peeringAttemptCount_(0),
	peeringAttemptMax_(5),
	port_(2691),
	bindAddress_("0.0.0.0"),
	brdAddress_("255.255.255.255"),
	backlog_(128),
	listenerWatcher_(loop_),
	connections_(),
	peeringWatcher_(loop_),
	modules_(),
	queues_(),
	terminateSignal_(loop_),
	interruptSignal_(loop_),
	commands_()
{
	//commands_["JOB PUSH SHELL"] = &Server::_pushShellCmd;

	terminateSignal_.set<Server, &Server::terminateSignal>(this);
	terminateSignal_.set(SIGTERM);
	terminateSignal_.start();
	loop_.unref();

	interruptSignal_.set<Server, &Server::interruptSignal>(this);
	interruptSignal_.set(SIGINT);
	interruptSignal_.start();
	loop_.unref();
}

Server::~Server()
{
	stop();
}

bool Server::setup(int argc, char* argv[])
{
	static const struct option long_options[] = {
		{ "help", no_argument, nullptr, 'h' },
		{ "port", required_argument, nullptr, 'p' },
		{ "bind-address", required_argument, nullptr, 'a' },
		{ "broadcast-address", required_argument, nullptr, 'b' },
		{ "cluster-group", required_argument, nullptr, 'g' },
		{ "standalone", no_argument, nullptr, 's' },
		{ 0, 0, 0, 0 }
	};

	bool standalone = false;

	for (bool done = false; !done;) {
		int long_index = 0;

		switch (getopt_long(argc, argv, "?hb:p:g:s", long_options, &long_index)) {
			case '?':
			case 'h':
				printHelp();
				return false;
			case 'a':
				bindAddress_ = optarg;
				break;
			case 'b':
				brdAddress_ = optarg;
				break;
			case 'p':
				port_ = atoi(optarg);
				break;
			case 'g':
				clusterGroup_ = optarg;
				break;
			case 's':
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

	if (!setupListener())
		return false;

	if (!setupPeeringListener())
		return false;

	if (standalone || !searchPeers())
		becomeMaster();

	return true;
}

void Server::stop()
{
	if (terminateSignal_.is_active()) {
		loop_.ref();
		terminateSignal_.stop();
	}

	if (interruptSignal_.is_active()) {
		loop_.ref();
		interruptSignal_.stop();
	}

	if (isMaster()) {
		// TODO promote another node to become master (once all jobs are done)
	} else {
		// TODO notify master that we're to quit (once all jobs are done)
	}

	if (peeringTimer_.is_active()) {
		peeringTimer_.stop();
	}

	if (peeringWatcher_.is_active()) {
		peeringWatcher_.stop();
		::close(peeringWatcher_.fd);
	}

	if (listenerWatcher_.is_active()) {
		listenerWatcher_.stop();
		::close(listenerWatcher_.fd);
	}
}

void Server::terminateSignal(ev::sig& sig, int revents)
{
	printf("Terminate signal received. Stopping\n");
	stop();
}

void Server::interruptSignal(ev::sig& sig, int revents)
{
	printf("Interrupt signal received. Stopping\n");
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
		" -a, --bind-address=IPADDR      local IP address to bind to [%s]\n"
		" -b, --broadcast-address=IPADDR remote IP/multicast/broadcast address to announce to [%s]\n"
		" -p, --port=NUMBER              port number for receiving/sending packets [%d]\n"
		" -k, --key=GROUP_KEY            cluster-group shared key [%s]\n"
		" -s, --standalone               do not broadcast for peering with cluster\n"
		"\n",
		PACKAGE_VERSION,
		homepageUrl,
		bindAddress_.c_str(),
		brdAddress_.c_str(),
		port_,
		clusterGroup_.c_str()
	);
}

// {{{ server network handling
bool Server::setupListener()
{
	std::printf("Setting up listener at tcp://%s:%d\n", bindAddress_.c_str(), port_);

	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return false;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port_);

	int rv;
	if ((rv = inet_pton(AF_INET, bindAddress_.c_str(), &sin.sin_addr.s_addr)) <= 0) {
		if (rv == 0)
			fprintf(stderr, "Address not in representation format.\n");
		else
			perror("inet_pton");
		close(fd);
		return false;
	}

	rv = bind(fd, (sockaddr*)&sin, sizeof(sin));
	if (rv < 0) {
		perror("bind");
		close(fd);
		return false;
	}

	rv = listen(fd, backlog_);
	if (rv < 0) {
		perror("listen");
		close(fd);
		return false;
	}

	listenerWatcher_.set<Server, &Server::incoming>(this);
	listenerWatcher_.set(fd, ev::READ);
	listenerWatcher_.start();

	return true;
}

void Server::incoming(ev::io& listener, int revents)
{
	std::printf("Server: new client connected\n");

	sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	int fd = accept(listener.fd, (sockaddr*)&sin, &slen);

	if (Connection* c = new Connection(this, fd)) {
		connections_.push_back(c);
	} else {
		close(fd);
	}
}

Connection* Server::unlink(Connection* connection)
{
	auto i = std::find(connections_.begin(), connections_.end(), connection);
	if (i != connections_.end())
		connections_.erase(i);

	return connection;
}
// }}}

// {{{ peering
bool Server::setupPeeringListener()
{
	std::printf("Setting up peering listener at udp://%s:%d\n", bindAddress_.c_str(), port_);

	int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return false;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port_);

	int rv;
	if ((rv = inet_pton(AF_INET, bindAddress_.c_str(), &sin.sin_addr.s_addr)) <= 0) {
		if (rv == 0)
			fprintf(stderr, "Address not in representation format.\n");
		else
			perror("inet_pton");
		close(fd);
		return false;
	}

	rv = bind(fd, (sockaddr*)&sin, sizeof(sin));
	if (rv < 0) {
		perror("bind");
		close(fd);
		return false;
	}

/*	rv = listen(fd, backlog_);
	if (rv < 0) {
		perror("listen");
		close(fd);
		return false;
	}
*/
	peeringWatcher_.set<Server, &Server::peering>(this);
	peeringWatcher_.set(fd, ev::READ);
	peeringWatcher_.start();

	return true;
}

bool Server::searchPeers()
{
	std::printf("Searching peeers via %s:%d\n", brdAddress_.c_str(), port_);
	state_ = State::BroadcastingForPeers;

	int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return false;
	}

	int val = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) < 0) {
		perror("setsockopt");
		return false;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port_);

	if ((val = inet_pton(AF_INET, brdAddress_.c_str(), &sin.sin_addr.s_addr)) <= 0) {
		if (val == 0)
			fprintf(stderr, "Address not in representation format.\n");
		else
			perror("inet_pton");
		return false;
	}

	char msg[128];
	ssize_t len = snprintf(msg, sizeof(msg), "HELLO %s", clusterGroup_.c_str());
	val = sendto(fd, msg, len, 0, (sockaddr*)&sin, sizeof(sin));

	if (val < 0) {
		perror("sendto");
		return false;
	}

	peeringTimer_.set<Server, &Server::peeringTimeout>(this);
	peeringTimer_.set(4.0, 0);
	peeringTimer_.start();

	return true;
}

void Server::peeringTimeout(ev::timer&, int revents)
{
	++peeringAttemptCount_;

	if (peeringAttemptCount_ < peeringAttemptMax_) {
		printf("Peering attempt (%d/%d) timed out. Retrying.\n", peeringAttemptCount_, peeringAttemptMax_);
		searchPeers();
	} else {
		printf("Peering attempt (%d/%d) timed out. Going standalone.\n", peeringAttemptCount_, peeringAttemptMax_);
		becomeMaster();
	}
}

void Server::becomeMaster()
{
	printf("Become master.\n");

	state_ = State::Running;
	schedulerRole_ = SchedulerRole::Master;
}

void Server::becomeSlave()
{
	printf("Become slave.\n");

	state_ = State::Running;
	schedulerRole_ = SchedulerRole::Slave;
}

void Server::peering(ev::io& io, int revents)
{
	char buf[4096];
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);

	ssize_t n = recvfrom(io.fd, buf, sizeof(buf), 0, (sockaddr*)&sin, &slen);
	if (n < 0) {
		printf("Invalid Peering packet received. %s\n", strerror(errno));
	} else if (n > 0) {
		buf[n] = '\0';
		char ip[16];
		if (!inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip)))
			strcpy(ip, "unknown");
		int port = ntohs(sin.sin_port);

		printf("Peering request received [%s:%d]: %s\n", ip, port, buf);
	}
}
// }}}
