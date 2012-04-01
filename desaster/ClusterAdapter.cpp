#include <desaster/ClusterAdapter.h>
#include <desaster/Server.h>

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

ClusterAdapter::ClusterAdapter(Server* server) :
	Module(server, "cluster"),
//	state_(State::Stopped),
//	schedulerRole_(SchedulerRole::Slave),
	groupName_("default"),
	bindAddress_("0.0.0.0"),
	brdAddress_("255.255.255.255"),
	port_(2692),
	peeringWatcher_(server->loop()),
	peeringTimer_(server->loop()),
	peeringAttemptTimeout_(5),
	peeringAttemptCount_(0),
	peeringAttemptMax_(5),
	syncWatcher_(server->loop()),
	connections_(),
	commands_()
{
}

ClusterAdapter::~ClusterAdapter()
{
	// promote another node as master if we are currently
	// otherwise notify master that we're shutting down hard.
}

bool ClusterAdapter::start()
{
	return true;
}

void ClusterAdapter::stop()
{
	if (peeringTimer_.is_active()) {
		peeringTimer_.stop();
	}

	if (peeringWatcher_.is_active()) {
		peeringWatcher_.stop();
		::close(peeringWatcher_.fd);
	}
}

bool ClusterAdapter::setupPeeringListener()
{
	notice("Setting up peering listener at udp://%s:%d", bindAddress_.c_str(), port_);

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
			error("Address not in representation format.");
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

	peeringWatcher_.set<ClusterAdapter, &ClusterAdapter::peering>(this);
	peeringWatcher_.set(fd, ev::READ);
	peeringWatcher_.start();

	return true;
}

bool ClusterAdapter::searchPeers()
{
	info("Searching peeers via %s:%d", brdAddress_.c_str(), port_);
	//state_ = State::BroadcastingForPeers;

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
			error("Address not in representation format.");
		else
			perror("inet_pton");
		return false;
	}

	char msg[128];
	ssize_t len = snprintf(msg, sizeof(msg), "HELLO %s", groupName_.c_str());
	val = sendto(fd, msg, len, 0, (sockaddr*)&sin, sizeof(sin));

	if (val < 0) {
		perror("sendto");
		return false;
	}

	peeringTimer_.set<ClusterAdapter, &ClusterAdapter::peeringTimeout>(this);
	peeringTimer_.set(4.0, 0);
	peeringTimer_.start();

	return true;
}

void ClusterAdapter::peeringTimeout(ev::timer&, int revents)
{
	++peeringAttemptCount_;

	if (peeringAttemptCount_ < peeringAttemptMax_) {
		notice("Peering attempt (%d/%d) timed out. Retrying.", peeringAttemptCount_, peeringAttemptMax_);
		searchPeers();
	} else {
		notice("Peering attempt (%d/%d) timed out. Going standalone.", peeringAttemptCount_, peeringAttemptMax_);
		becomeMaster();
	}
}

void ClusterAdapter::becomeMaster()
{
	notice("Become master.");

//	state_ = State::Running;
//	schedulerRole_ = SchedulerRole::Master;
}

void ClusterAdapter::becomeSlave()
{
	notice("Become slave.");

//	state_ = State::Running;
//	schedulerRole_ = SchedulerRole::Slave;
}

void ClusterAdapter::peering(ev::io& io, int revents)
{
	char buf[4096];
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);

	ssize_t n = recvfrom(io.fd, buf, sizeof(buf), 0, (sockaddr*)&sin, &slen);
	if (n < 0) {
		error("Invalid Peering packet received. %s", strerror(errno));
	} else if (n > 0) {
		buf[n] = '\0';
		char ip[16];
		if (!inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip)))
			strcpy(ip, "unknown");
		int port = ntohs(sin.sin_port);

		debug("Peering request received [%s:%d]: %s", ip, port, buf);
	}
}
