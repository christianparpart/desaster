#include <desaster/Server.h>
#include <ev++.h>

int main(int argc, char* argv[])
{
	ev::default_loop loop;
	Server server(loop);

	if (!server.start(argc, argv))
		return 1;

	loop.run();

	return 0;
}
