#include <desaster/Worker.h>
#include <desaster/Queue.h>
#include <desaster/Job.h>

// invoked by scheduler, to assign this worker a new job
void Worker::assign(Job* job)
{
	job_ = job;
	perform();
}

void Worker::finish(bool success)
{
	job_->queue()->notifyComplete(job_, success);
	job_ = nullptr;
}
