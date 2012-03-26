#ifndef desaster_Job_h
#define desaster_Job_h (1)

#include <string>

class Job
{
public:
	Job();
	virtual ~Job();

	virtual std::string str() const = 0;
	virtual std::string serialize() const = 0;
};

#endif
