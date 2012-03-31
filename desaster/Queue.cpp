#include <desaster/Queue.h>
#include <desaster/Job.h>
#include <algorithm>
#include <cstdio>

Queue::Queue(Server& server, const std::string& name) :
	server_(server),
	name_(name),
	bucket_(nullptr),
	jobs_()
{
	std::printf("Queue[%s]: created\n", name_.c_str());
}

Queue::~Queue()
{
}

void Queue::enqueue(Job* job)
{
	job->queue_ = this;
	jobs_.push_back(job);
	std::printf("Queue[%s]: enqueueing job: %s\n", name_.c_str(), job->str().c_str());
}

Job* Queue::dequeue()
{
	Job* job = jobs_.front();
	jobs_.pop_front();
	std::printf("Queue[%s]: dequeueing job: %s\n", name_.c_str(), job->str().c_str());
	return job;
}

void Queue::notifyComplete(Job* job, bool success)
{
	std::printf("Queue[%s]: Job completed: %s\n", name_.c_str(), job->str().c_str());
	delete job;
}

Job* Queue::unlink(Job* job)
{
	auto i = std::find(jobs_.begin(), jobs_.end(), job);

	if (i != jobs_.end())
		jobs_.erase(i);

	return job;
}

