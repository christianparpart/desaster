#ifndef desaster_Queue_h
#define desaster_Queue_h (1)

#include <deque>
#include <desaster/qdisc.h>
#include <desaster/Logging.h>

class Job;
class Server;
class Worker;

class Queue :
	public Logging
{
private:
	Server& server_;
	std::string name_;
	qdisc::htb::node* bucket_;
	std::deque<Job*> jobs_;

	friend class Job;
	friend class Worker;

public:
	Queue(Server& server, const std::string& name);
	~Queue();

	Server& server() const { return server_; }
	const std::string& name() const { return name_; }

	void enqueue(Job* job);
	Job* dequeue();

	bool empty() const { return jobs_.empty(); }
	size_t size() const { return jobs_.size(); }

	// shaping
	size_t actualRate() const { return bucket_->actual_rate(); }
	size_t rate() const { return bucket_->rate(); }
	size_t ceil() const { return bucket_->ceil(); }

	qdisc::htb::node* bucket() const { return bucket_; }

private:
	void notifyComplete(Job* job, bool success);
	Job* unlink(Job* job);
};

#endif
