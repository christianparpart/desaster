#ifndef desaster_shell_ShellJob_h
#define desaster_shell_ShellJob_h (1)

#include <desaster/Job.h>

class ShellJob :
	public Job
{
private:
	std::string command_;

public:
	ShellJob(const std::string& cmd);
	~ShellJob();

	const std::string& command() const { return command_; }

	virtual std::string str() const;
	virtual std::string serialize() const;
};

#endif
