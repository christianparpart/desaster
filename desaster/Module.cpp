#include <desaster/Module.h>

Module::Module(Server* server, const std::string& name) :
	Logging(name.c_str()),
	server_(server),
	name_(name)
{
}

Module::~Module()
{
}

bool Module::start()
{
	return true;
}

void Module::stop()
{
}
