#ifndef desaster_Module_h
#define desaster_Module_h (1)

#include <desaster/Logging.h>
#include <unordered_map>
#include <string>

class Server;
class Job;

class Module :
	public Logging
{
protected:
	Server* server_;
	std::string name_;

public:
	Module(Server*, const std::string& name);
	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;
	virtual ~Module();

	virtual bool start();
	virtual void stop();

	Server& server() const { return *server_; }
	const std::string& name() const { return name_; }

	//virtual Job* createJob(const std::string& serialized) = 0;
};

#endif
