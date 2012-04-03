#include <desaster/Job.h>
#include <desaster/Queue.h>

Job::Job(Queue* queue, unsigned long long id, const std::string& payload) :
	queue_(queue),
	id_(id),
	enqueuedAt_(0),
	startedAt_(0),
	finishedAt_(0),
	payload_(payload)
{
}

Job::~Job()
{
	queue_->unlink(this);
}

std::string Job::str() const
{
	char buf[128];
	ssize_t n = snprintf(buf, sizeof(buf), "Job:%llu", id_);
	return std::string(buf, 0, n);
}
