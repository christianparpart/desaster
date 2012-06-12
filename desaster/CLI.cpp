#include <desaster/CLI.h>
#include <cstring>

// TODO
// - support interactive shell via readline() or (client-)compatible.
// - support automatic generation of help text.
// - support automatic parameter checking (existence, type)

namespace desaster {

// {{{ CLI
CLI::CLI()
{
}

CLI::~CLI()
{
	for (auto group: groups_)
		delete group;
}

CLI::Factory CLI::create()
{
	return Factory(*this);
}

CLI::Group* CLI::find(const std::string& groupName)
{
	for (auto group: groups_)
		if (group->name() == groupName)
			return group;

	// do not permit multiple command groups to match
	std::vector<Group*> multi;
	for (auto group: groups_)
		if (strncmp(group->name().c_str(), groupName.c_str(), groupName.size()) == 0)
			multi.push_back(group);

	if (multi.size() == 1)
		return multi[0];

	if (multi.size() > 1)
		throw MultipleCommandsMatchError();

	return nullptr;
}

int CLI::evaluate(int argc, char* argv[])
{
	if (argc > 0) {
		if (Group* group = find(argv[0])) {
			if (Command* command = group->find(argv[1])) {
				std::printf("evaluate: %s %s\n", group->name().c_str(), command->name().c_str());
				return command->invoke(argc - 2, argv + 2);
			}
		}

		if (Group* group = find("")) {
			if (Command* command = group->find(argv[0])) {
				std::printf("evaluate: %s\n", command->name().c_str());
				return command->invoke(argc - 1, argv + 1);
			}
		}
	}

	throw NotFoundError();
}
// }}}
// {{{ CLI::Group
CLI::Group::Group(const std::string& name) :
	name_(name),
	commands_()
{
}

CLI::Group::~Group()
{
	for (auto command: commands_)
		delete command;
}

void CLI::Group::define(const std::string& commandName, const Callback& callback, const std::string& helpText)
{
	commands_.push_back(new Command(commandName, callback, helpText));
}

void CLI::Group::remove(const std::string& commandName)
{
	for (auto i = commands_.begin(), e = commands_.end(); i != e; ++i) {
		Command* command = *i;
		if (command->name() == commandName) {
			commands_.erase(i);
			return;
		}
	}
}

CLI::Command* CLI::Group::find(const std::string& commandName)
{
	for (auto command: commands_)
		if (command->name() == commandName)
			return command;

	std::vector<Command*> multi;
	for (auto command: commands_)
		if (strncmp(command->name().c_str(), commandName.c_str(), commandName.size()) == 0)
			multi.push_back(command);

	// do not permit multiple commands to match
	if (multi.size() == 1)
		return multi[0];

	if (multi.size() > 1)
		throw MultipleCommandsMatchError();

	return nullptr;
}
// }}}
// {{{ CLI::Command
CLI::Command::Command(const std::string& name, const Callback& callback, const std::string& helpText) :
	name_(name),
	callback_(callback),
	helpText_(helpText)
{
}

CLI::Command::~Command()
{
}

int CLI::Command::invoke(int argc, char *argv[])
{
	Params params;

	for (int i = 0; i < argc; ++i)
		params.push_back(argv[i]);

	return callback_(params);
}
// }}}
// {{{ CLI::Factory
CLI::Factory::Factory(CLI& cli) :
	cli_(cli),
	currentGroup_(nullptr)
{
}

CLI::Factory CLI::Factory::group(const std::string& name)
{
	for (auto group: cli_.groups_) {
		if (group->name() == name) {
			currentGroup_ = group;
			return *this;
		}
	}

	currentGroup_ = new Group(name);
	cli_.groups_.push_back(currentGroup_);
	return *this;
}

CLI::Factory CLI::Factory::command(const std::string& name)
{
	return command(
		name,
		[](const Params&) -> int { return 0; },
		"not yet implemented"
	);
}

CLI::Factory CLI::Factory::command(const std::string& name, const Callback& callback, const std::string& helpText)
{
	currentGroup_->remove(name);
	currentGroup_->define(name, callback, helpText);

	return *this;
}
// }}}

} // namespace desaster
