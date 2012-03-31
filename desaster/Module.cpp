#include <desaster/Module.h>

Module::Module(Server* server, const std::string& name) :
	server_(server),
	name_(name),
	commands_()
{
}

Module::~Module()
{
}

void Module::handleCommand(const Connection::Message& message)
{
	auto c = commands_.find(message.command());
	if (c == commands_.end()) {
		(this->*(c->second))(message);
	}
}
