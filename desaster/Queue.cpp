#include <desaster/Queue.h>
#include <desaster/Server.h>
#include <desaster/Job.h>
#include <algorithm>
#include <cstdio>

Queue::Queue(Server& server, const std::string& name) :
	Logging("Queue/%s", name.c_str()),
	server_(server),
	name_(name),
	bucket_(server_.rootBucket().create_child(name)),
	jobs_()
{
	debug("created");
}

Queue::~Queue()
{
	debug("destroying");
}

void Queue::enqueue(Job* job)
{
	job->queue_ = this;
	jobs_.push_back(job);

	debug("enqueueing job: %s", job->str().c_str());
}

/*! dequeues a job in case there is at least one job and one bucket available.
 *
 * \return pointer to the job dequeued or nullptr.
 */
Job* Queue::dequeue()
{
	if (empty() || !bucket_->get(1))
		return nullptr;

	Job* job = jobs_.front();
	jobs_.pop_front();

	debug("dequeueing job: %s", job->str().c_str());

	return job;
}

void Queue::notifyComplete(Job* job, bool success)
{
	notice("Job completed (%s): %s", success ? "success" : "failed", job->str().c_str());
	bucket_->put(1);
	delete job;
}

Job* Queue::unlink(Job* job)
{
	auto i = std::find(jobs_.begin(), jobs_.end(), job);

	if (i != jobs_.end())
		jobs_.erase(i);

	return job;
}

