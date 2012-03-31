#ifndef desaster_shell_ShellJob_h
#define desaster_shell_ShellJob_h (1)

#include <desaster/Job.h>

class ShellModule;

class ShellJob :
	public Job
{
private:
	ShellModule* module_;

public:
	ShellJob(Queue* owner, const std::string& cmd);
	~ShellJob();

	virtual std::string str() const;
	virtual std::string serialize() const;
	virtual void perform();
};

#endif
