#ifndef desaster_Job_h
#define desaster_Job_h (1)

#include <string>

class Queue;

class Job
{
protected:
	Queue* queue_;
	unsigned long long id_;
	time_t enqueuedAt_;
	time_t startedAt_;
	time_t finishedAt_;

	friend class Queue;

public:
	Job(Queue* queue, unsigned long long id);
	virtual ~Job();

	Queue* queue() const { return queue_; }
	unsigned long long id() const { return id_; }

	time_t enqueuedAt() const { return enqueuedAt_; }
	time_t startedAt() const { return startedAt_; }
	time_t finishedAt() const { return finishedAt_; }

	virtual std::string str() const;
	virtual std::string serialize() const;
	virtual void perform();

	void finish(bool success);
};

#endif
