#include <desaster/Job.h>
#include <desaster/Queue.h>

Job::Job(Queue* queue, unsigned long long id) :
	queue_(queue),
	id_(id)
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

std::string Job::serialize() const
{
	return "OVERRIDE-ME";
}

void Job::perform()
{
	// override me
}

void Job::finish(bool success)
{
	queue_->notifyComplete(this, success);
}
