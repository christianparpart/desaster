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
	ev::child childWatcher_;
	int child_;
	Pipe output_;

public:
	explicit ShellWorker(Server* server);
	~ShellWorker();

	virtual void perform();

private:
	void setupChild();
	void setupParent();
	void onChild(ev::child& child, int revents);
};

#endif
