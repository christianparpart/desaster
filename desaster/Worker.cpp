#include "Worker.h"
#include "Job.h"

// invoked by scheduler, to assign this worker a new job
void Worker::assign(Job* job)
{
	job_ = job;
	perform();
}

void Worker::finish(bool success)
{
	job_ = nullptr;
	// TODO notify scheduler about state change
}
