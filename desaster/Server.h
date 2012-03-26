#ifndef desaster_Server_h
#define desaster_Server_h (1)

#include <string>
#include <vector>
#include <unordered_map>
#include <ev++.h>

class Job;
class Worker;
class ShellWorker;

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
	std::string bindAddress_;
	std::string brdAddress_;

	// workers
	std::vector<Worker*> allWorkers_;
	std::vector<ShellWorker*> shellWorkers_;

	std::unordered_map<std::string, void (Server::*)(int fd, std::string& args)> commands_;

public:
	explicit Server(ev::loop_ref loop);
	~Server();

	bool setup(int argc, char* argv[]);

	void enqueue(Job* job);
	Job* dequeue();

	ShellWorker* spawnShellWorker();

private:
	void printHelp(const char* program);
	bool searchPeers(int port, const std::string& brdAddress);
	void peeringTimeout(ev::timer&, int);

	void onJob(ev::io& io, int revents);

	// commands
	void _pushShellCmd(int fd, const std::string& args);
};

#endif
