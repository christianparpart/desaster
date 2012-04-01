#ifndef desaster_Server_h
#define desaster_Server_h (1)

#include <desaster/Logging.h>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <ev++.h>

class Job;
class Queue;
class Module;

class Server :
	public Logging
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

	std::list<Module*> modules_;
	std::vector<Queue*> queues_;

	ev::sig terminateSignal_;
	ev::sig interruptSignal_;

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
	Queue* findQueue(const std::string& name) const;

	Module* registerModule(Module* module);
	Module* unregisterModule(Module* module);

	template<typename T> T* module() const { // {{{
		for (auto module: modules_)
			if (T* u = dynamic_cast<T*>(module))
				return u;

		return nullptr;
	} // }}}

private:
	Queue* unlink(Queue* queue);
	Module* unlink(Module* module);

private:
	void printHelp();

	void terminateSignal(ev::sig& signal, int revents);
	void interruptSignal(ev::sig& signal, int revents);
};

#endif
