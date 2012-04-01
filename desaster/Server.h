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
	ev::loop_ref loop_;
	std::string logFileName_;
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

	bool start(int argc, char* argv[]);
	void stop();

	Module* registerModule(Module* module);
	Module* unregisterModule(Module* module);
	template<typename T> T* module() const;

	Queue* createQueue(const std::string& name);
	Queue* findQueue(const std::string& name) const;

private:
	Module* unlink(Module* module);
	Queue* unlink(Queue* queue);

private:
	void printHelp();

	void terminateSignal(ev::sig& signal, int revents);
	void interruptSignal(ev::sig& signal, int revents);
};

template<typename T>
T* Server::module() const
{
	for (auto module: modules_)
		if (T* u = dynamic_cast<T*>(module))
			return u;

	return nullptr;
}

#endif
