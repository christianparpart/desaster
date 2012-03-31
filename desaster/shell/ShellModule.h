#ifndef desaster_shell_ShellModule_h
#define desaster_shell_ShellModule_h (1)

#include <desaster/Module.h>

class ShellModule :
	public Module
{
private:
	// child process
	pid_t child_;
	ev::child childWatcher_;

	// non-blocking logging pipes
	Pipe output_;
	ev::io outputWatcher_;

public:
	explicit ShellModule(Server* server);
	~ShellModule();

	virtual Job* createJob(const std::string& serialized);
	virtual void perform(Job*);

private:
	void setupParent();
	void setupChild();
	void onOutput(ev::io&, int);
	void onChild(ev::child&, int);
};

#endif
