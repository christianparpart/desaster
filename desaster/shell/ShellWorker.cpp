#include <desaster/shell/ShellWorker.h>
#include <desaster/Job.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

void ShellWorker::perform()
{
	output_.clear();

	child_ = vfork();
	switch (child_) {
		case -1: // error
			perror("vfork");
			finish(false);
			break;
		case 0: // in child
			setupChild();
			break;
		default: // in parent
			setupParent();
			break;
	}
}

void ShellWorker::setupChild()
{
	close(0);
	dup2(open("/dev/null", O_RDONLY), 0);

	close(STDOUT_FILENO);
	dup2(output_.writeFd(), STDOUT_FILENO);

	close(STDERR_FILENO);
	dup2(output_.writeFd(), STDERR_FILENO);

	char** argv = new char* [2];
	argv[0] = const_cast<char*>(job()->payload().c_str());
	argv[1] = nullptr;

	execvp(job()->payload().c_str(), argv);

	// only reached on error.
	perror("execvp");
	delete[] argv;
	_exit(1);
}

void ShellWorker::setupParent()
{
	childWatcher_.set(child_, false);
	childWatcher_.set<ShellWorker, &ShellWorker::onChild>(this);
	childWatcher_.start();
}

void ShellWorker::onChild(ev::child&, int)
{
	printf("child (%d) state changed 0x%04X\n", child_, childWatcher_.rstatus);
}
