#include "Server.h"
#include <getopt.h>
#include <iostream>

Server::Server(ev::loop_ref loop) :
	loop_(loop)
{
}

Server::~Server()
{
}

bool Server::setup(int argc, char* argv[])
{
	static const struct option long_options[] = {
		{ "help", no_argument, nullptr, 'h' },
		{ 0, 0, 0, 0 }
	};

	for (;;) {
		int long_index = 0;

		switch (getopt_long(argc, argv, "?h", long_options, &long_index)) {
			case '?':
			case 'h':
				std::printf("usage: %s\n", argv[0]);
				return false;
			case 0:
				// long opt with val != NULL
				break;
			case -1: // EOF
				printf("done\n");
				return true;
			default:
				return false;
		}
	}
}

void Server::io(ev::io& io, int revents)
{
	char tmp[80];
	std::cin.getline(tmp, sizeof(tmp));
	printf("%f\n", loop_.now());
}
