#ifndef desaster_shell_ShellWorker_h
#define desaster_shell_ShellWorker_h

#include <desaster/Worker.h>
#include <desaster/Pipe.h>
#include <ev++.h>

// the shell worker forks on demand for every command to perform
class ShellWorker :
	public Worker
{
private:
	// child process
	pid_t child_;
	ev::child childWatcher_;

	// non-blocking logging pipes
	Pipe output_;
	ev::io outputWatcher_;

public:
	explicit ShellWorker(Server* server);
	~ShellWorker();

	virtual void perform();

private:
	void setupParent();
	void setupChild();
	void onOutput(ev::io&, int);
	void onChild(ev::child&, int);
};

#endif
