#ifndef desaster_Worker_h
#define desaster_Worker_h

#include "Pipe.h"
#include "ev++.h"

class Server;
class Job;

class Worker
{
protected:
	Server* server_;
	Job* job_;

public:
	explicit Worker(Server* server);
	virtual ~Worker();

	void assign(Job* job);

	template<typename T>
	const T* job() const { return dynamic_cast<const T*>(job_); }

	bool busy() const { return job_ != nullptr; }
	bool idle() const { return job_ == nullptr; }

protected:
	virtual void perform() = 0;

	void log(const std::string& message);
	void log(const char* buf, size_t n);
	void finish(bool success);
};

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

// the ruby worker pre-forks into a ruby-environment, accepting incoming
// synchronous requests over a communication line (stdin) and reports
// to stdout/stderr.
class RubyWorker :
	public Worker
{
private:
	// child process
	pid_t child_;
	ev::child childWatcher_;

	// non-blocking logging pipes (connected to stdout + stderr)
	Pipe output_;
	ev::io outputWatcher_;

	// synchronous command pipe (connected to stdin)
	Pipe input_;
};

#endif
