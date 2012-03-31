#ifndef desaster_Module_h
#define desaster_Module_h (1)

#include <string>
#include <unordered_map>
#include <desaster/Connection.h>

class Server;
class Job;

class Module
{
protected:
	Server* server_;
	std::string name_;

	std::unordered_map<std::string, void (Module::*)(const Connection::Message&)> commands_;

public:
	Module(Server*, const std::string& name);
	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;
	virtual ~Module();

	Server& server() const { return *server_; }
	const std::string& name() const { return name_; }

	void handleCommand(const Connection::Message& message);

	virtual Job* createJob(const std::string& serialized) = 0;
};

#endif
