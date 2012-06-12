#pragma once

#include <string>
#include <list>
#include <vector>
#include <functional>
#include <exception>

namespace desaster {

class CLI
{
public:
	class Factory;
	class Group;
	class Command;

	class NotFoundError;
	class MultipleCommandsMatchError;

	typedef std::vector<std::string> Params;
	typedef std::function<int(const Params&)> Callback;

public:
	CLI();
	~CLI();

	Factory create();

	size_t size() const { return groups_.size(); }
	std::list<Group*>::iterator begin() { return groups_.begin(); }
	std::list<Group*>::iterator end() { return groups_.end(); }

	int evaluate(int argc, char* argv[]);

	Group* find(const std::string& groupName);

private:
	std::list<Group*> groups_;
};

class CLI::Group
{
public:
	explicit Group(const std::string& name);
	~Group();

	const std::string& name() const { return name_; }

	void define(const std::string& command, const Callback& callback, const std::string& helpText);
	void remove(const std::string& command);

	Command* find(const std::string& commandName);

	size_t size() const { return commands_.size(); }
	std::list<Command*>::iterator begin() { return commands_.begin(); }
	std::list<Command*>::iterator end() { return commands_.end(); }

private:
	std::string name_;
	std::list<Command*> commands_;
};

class CLI::Command
{
public:
	Command(const std::string& name, const Callback& callback, const std::string& helpText);
	~Command();

	const std::string& name() const { return name_; }
	const std::string& helpText() const { return helpText_; }

	int invoke(int argc, char *argv[]);

private:
	std::string name_;
	Callback callback_;
	std::string helpText_;
};

class CLI::Factory
{
private:
	CLI& cli_;
	Group* currentGroup_;

public:
	explicit Factory(CLI& cli);

	Factory group(const std::string& name = "");

	Factory command(const std::string& name);
	Factory command(const std::string& name, const Callback& callback, const std::string& helpText = "");
};

class CLI::NotFoundError : public std::exception
{
public:
	NotFoundError() {}

	const char* what() const throw() { return "Command not found."; }
};

class CLI::MultipleCommandsMatchError : public std::exception
{
public:
	MultipleCommandsMatchError() {}

	const char* what() const throw() { return "Multiple commands matched."; }
};

} // namespace desaster
