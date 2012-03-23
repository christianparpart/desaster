#ifndef desaster_Server_h
#define desaster_Server_h (1)

#include <ev++.h>

class Server
{
private:
	ev::loop_ref loop_;

public:
	explicit Server(ev::loop_ref loop);
	~Server();

	bool setup(int argc, char* argv[]);

	void io(ev::io& io, int revents);
};

#endif
