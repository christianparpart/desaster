#ifndef desaster_Server_h
#define desaster_Server_h (1)

#include <ev++.h>

class Server
{
private:
	enum class State {
		Stopped,
		BroadcastingForPeers,
		Running,
		ShuttingDown,
	};

	enum class SchedulerRole {
		Active,
		Passive
	};

	ev::loop_ref loop_;

	int peeringListener_;
	ev::timer peeringTimer_;
	int peeringAttemptTimeout_;
	int peeringAttemptCount_;
	int peeringAttemptMax_;

	int port_;
	std::string ipaddr_;
	std::string brdaddr_;

public:
	explicit Server(ev::loop_ref loop);
	~Server();

	bool setup(int argc, char* argv[]);

private:
	void printHelp(const char* program);
	bool searchPeers(int port, const std::string& brdaddr);
	void peeringTimeout(ev::timer&, int);

	void onJob(ev::io& io, int revents);
};

#endif
