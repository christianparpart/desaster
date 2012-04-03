#ifndef desaster_Worker_h
#define desaster_Worker_h

#include <desaster/Logging.h>
#include <ev++.h>

class Server;
class Job;

class Worker :
	public Logging
{
protected:
	Server* server_;
	Job* job_;

public:
	explicit Worker(Server* server);
	virtual ~Worker();

	void assign(Job* job);
	const Job* job() const { return job_; }

	bool busy() const { return job_ != nullptr; }
	bool idle() const { return job_ == nullptr; }

protected:
	virtual void perform() = 0;

	void log(const std::string& message);
	void log(const char* buf, size_t n);
	void finish(bool success);
};

#if 0
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

#endif
