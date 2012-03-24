#ifndef desaster_Job_h
#define desaster_Job_h (1)

#include <string>

class Job
{
public:
	Job();
	virtual ~Job();

	virtual std::string str() const = 0;
};

class ShellJob : public Job
{
private:
	std::string command_;

public:
	ShellJob(const std::string& cmd);
	~ShellJob();

	const std::string& command() const { return command_; }

	virtual std::string str() const;
};

// invokes a ruby method: SomeClass.perform(*arguments)
class RubyJob : public Job
{
private:
	std::string className_;
	std::string arguments_;

public:
	RubyJob(const std::string& className, const std::string& args) :
		className_(className), arguments_(args) {}

	const std::string& className() const { return className_; }
	const std::string& arguments() const { return arguments_; }

	virtual std::string str() const;
};

#endif
