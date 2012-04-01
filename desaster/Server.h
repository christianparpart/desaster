#ifndef desaster_Server_h
#define desaster_Server_h (1)

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <ev++.h>

class Job;
class Queue;
class Module;
class Connection;

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
		Master,
		Slave
	};

	State state_;
	SchedulerRole schedulerRole_;
	std::string clusterGroup_;

	ev::loop_ref loop_;

	ev::io peeringWatcher_;
	ev::timer peeringTimer_;
	int peeringAttemptTimeout_;
	int peeringAttemptCount_;
	int peeringAttemptMax_;

	int port_;
	std::string bindAddress_;
	std::string brdAddress_;

	int backlog_;
	ev::io listenerWatcher_;
	std::list<Connection*> connections_;

	std::list<Module*> modules_;
	std::vector<Queue*> queues_;

	ev::sig terminateSignal_;
	ev::sig interruptSignal_;

	std::unordered_map<std::string, void (Server::*)(Connection* remote, const std::string& args)> commands_;

	friend class Connection;
	friend class Module;
	friend class Queue;

public:
	explicit Server(ev::loop_ref loop);
	~Server();

	ev::loop_ref loop() const { return loop_; }

	bool setup(int argc, char* argv[]);
	void stop();

	bool isMaster() const { return schedulerRole_ == SchedulerRole::Master; }

	Queue* createQueue(const std::string& name);

	void enqueue(Job* job);
	Job* dequeue();

	void registerModule(Module* module);
	void unregisterModule(Module* module);

private:
	Connection* unlink(Connection* connection);
	Queue* unlink(Queue* queue);
	Module* unlink(Module* module);

private:
	void printHelp();
	bool setupListener();
	bool setupPeeringListener();
	bool searchPeers();
	void peeringTimeout(ev::timer&, int);
	void becomeMaster();
	void becomeSlave();

	void peering(ev::io& listener, int revents);
	void incoming(ev::io& listener, int revents);
	void onJob(ev::io& io, int revents);

	void terminateSignal(ev::sig& signal, int revents);
	void interruptSignal(ev::sig& signal, int revents);

	// commands
	void _pushShellCmd(int fd, const std::string& args);
};

#endif
