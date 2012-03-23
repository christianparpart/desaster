#include "Server.h"
#include <ev++.h>
#include <cstdio>

int main(int argc, char* argv[])
{
	ev::default_loop loop;
	Server server(loop);

	if (!server.setup(argc, argv))
		return 1;

	ev::io io(loop);
	io.set<Server, &Server::io>(&server);
	io.set(0, ev::READ);
	io.start();

	loop.run();

	return 0;
}
