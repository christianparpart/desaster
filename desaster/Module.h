#ifndef desaster_Module_h
#define desaster_Module_h (1)

#include <string>

class Server;
class Worker;
class Job;

class Module
{
protected:
	Server* server_;
	std::string name_;

public:
	Module(Server*, const std::string& name);
	virtual ~Module();

	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;

	Server& server() const { return *server_; }
	const std::string& name() const { return name_; }

	virtual Job* createJob() = 0;
	virtual Worker* createWorker() = 0;
};

#endif
