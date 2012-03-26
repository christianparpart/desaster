#include <desaster/Module.h>

Module::Module(Server* server, const std::string& name) :
	server_(server),
	name_(name)
{
}

Module::~Module()
{
}

