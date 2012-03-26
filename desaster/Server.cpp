#include "Server.h"
#include <iostream>
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
	loop_(loop),
	peeringTimer_(loop_),
	peeringAttemptTimeout_(5), // seconds
	peeringAttemptCount_(0),
	peeringAttemptMax_(5),
	port_(2691),
	bindAddress_("0.0.0.0"),
	brdAddress_("255.255.255.255"),
	commands_()
{
	commands_["JOB PUSH SHELL"] = &Server::_pushShellCmd;
}

Server::~Server()
{
}

bool Server::setup(int argc, char* argv[])
{
	static const struct option long_options[] = {
		{ "help", no_argument, nullptr, 'h' },
		{ "port", required_argument, nullptr, 'p' },
		{ "bind-address", required_argument, nullptr, 'a' },
		{ "broadcast-address", required_argument, nullptr, 'b' },
		{ 0, 0, 0, 0 }
	};

	std::string bindAddress;
	std::string brdAddress;
	int port = -1;

	for (bool done = false; !done;) {
		int long_index = 0;

		switch (getopt_long(argc, argv, "?hb:p:", long_options, &long_index)) {
			case '?':
			case 'h':
				printHelp(argv[0]);
				return false;
			case 'a':
				bindAddress = optarg;
				break;
			case 'b':
				brdAddress = optarg;
				break;
			case 'p':
				port = atoi(optarg);
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

	if (port < 0)
		port = 2691;

	if (brdAddress.empty())
		brdAddress = brdAddress_;

	if (bindAddress.empty())
		bindAddress = bindAddress_;

	return searchPeers(port, brdAddress);
}

void Server::printHelp(const char* program)
{
	printf(
		" usage %s [-a BIND_ADDR] [-b BROADCAST_ADDR] [-p PORT]\n"
		"\n"
		" -?, -h, --help                 prints this help\n"
		" -a, --bind-address=IPADDR      local IP address to bind to [%s]\n"
		" -b, --broadcast-address=IPADDR remote IP/multicast/broadcast address to announce to [%s]\n"
		" -p, --port=NUMBER              port number for receiving/sending packets [%d]\n"
		" -k, --key=GROUP_KEY            cluster-group shared key [%s]\n"
		"\n",
		program,
		bindAddress_.c_str(),
		brdAddress_.c_str(),
		port_,
		"default",
	);
}

// {{{ peering
bool Server::searchPeers(int port, const std::string& brdAddress)
{
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
	sin.sin_port = htons(port);

	if ((val = inet_pton(AF_INET, brdAddress.c_str(), &sin.sin_addr.s_addr)) <= 0) {
		if (val == 0)
			fprintf(stderr, "Address not in representation format.\n");
		else
			perror("inet_pton");
		return false;
	}

	std::string message = "HELLO";
	val = sendto(fd, message.data(), message.size(), 0,
		(sockaddr*)&sin, sizeof(sin));

	if (val < 0) {
		perror("sendto");
		return false;
	}

	port_ = port;
	brdAddress_ = brdAddress;

	peeringTimer_.set<Server, &Server::peeringTimeout>(this);
	peeringTimer_.set(4.0, 0);
	peeringTimer_.start();

	return true;
}

void Server::peeringTimeout(ev::timer&, int revents)
{
	printf("peering attempt timed out.\n");

	++peeringAttemptCount_;
	if (peeringAttemptTimeout_ < peeringAttemptMax_) {
		searchPeers(port_, brdAddress_);
	}
}
// }}}
