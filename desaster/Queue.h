#ifndef desaster_Queue_h
#define desaster_Queue_h (1)

#include <deque>
#include <desaster/qdisc.h>

class Job;
class Server;

class Queue
{
private:
	Server& server_;
	std::string name_;
	qdisc::bucket* bucket_;
	std::deque<Job*> jobs_;

	friend class Job;

public:
	Queue(Server& server, const std::string& name);
	~Queue();

	Server& server() const { return server_; }
	const std::string& name() const { return name_; }

	void enqueue(Job* job);
	Job* dequeue();

	size_t size() const { return jobs_.size(); }

	qdisc::bucket* bucket() const { return bucket_; }
	void setBucket(qdisc::bucket* bucket) { bucket_ = bucket; }

private:
	void notifyComplete(Job* job, bool success);
	Job* unlink(Job* job);
};

#endif
