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
	ipaddr_("0.0.0.0"),
	brdaddr_("255.255.255.255")
{
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

	std::string ipaddr;
	std::string brdaddr;
	int port = -1;

	for (bool done = false; !done;) {
		int long_index = 0;

		switch (getopt_long(argc, argv, "?hb:p:", long_options, &long_index)) {
			case '?':
			case 'h':
				printHelp(argv[0]);
				return false;
			case 'a':
				ipaddr = optarg;
				break;
			case 'b':
				brdaddr = optarg;
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

	if (brdaddr.empty())
		brdaddr = brdaddr_;

	if (ipaddr.empty())
		ipaddr = ipaddr_;

	return searchPeers(port, brdaddr);
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
		"\n",
		program,
		ipaddr_.c_str(),
		brdaddr_.c_str(),
		port_
	);
}

bool Server::searchPeers(int port, const std::string& brdaddr)
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

	if ((val = inet_pton(AF_INET, brdaddr.c_str(), &sin.sin_addr.s_addr)) <= 0) {
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
	brdaddr_ = brdaddr;

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
		searchPeers(port_, brdaddr_);
	}
}

