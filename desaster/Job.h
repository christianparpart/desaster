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
	std::string payload_;

	friend class Queue;

public:
	Job(Queue* queue, unsigned long long id, const std::string& payload);
	~Job();

	Queue* queue() const { return queue_; }
	unsigned long long id() const { return id_; }
	const std::string& payload() const { return payload_; }

	time_t enqueuedAt() const { return enqueuedAt_; }
	time_t startedAt() const { return startedAt_; }
	time_t finishedAt() const { return finishedAt_; }

	std::string str() const;
};

#endif
