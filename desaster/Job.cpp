#include "Job.h"
#include <sstream>

Job::Job()
{
}

Job::~Job()
{
}

ShellJob::ShellJob(const std::string& cmd) :
	command_(cmd)
{
}

ShellJob::~ShellJob()
{
}

std::string ShellJob::str() const
{
	std::stringstream sstr;
	sstr << "Shell:" << command_;
	return std::move(sstr.str());
}

std::string RubyJob::str() const
{
	std::stringstream sstr;
	sstr << "Ruby:" << className_ << ".perform(" << arguments_ << ")";
	return std::move(sstr.str());
}
