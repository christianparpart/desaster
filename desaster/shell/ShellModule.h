#ifndef desaster_shell_ShellModule_h
#define desaster_shell_ShellModule_h (1)

#include <desaster/Module.h>

class ShellModule :
	public Module
{
public:
	explicit ShellModule(Server* server);
	~ShellModule();

	virtual Job* createJob();
	virtual Worker* createWorker();
};

#endif
