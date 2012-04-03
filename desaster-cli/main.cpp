#include <string>
#include <initializer_list>
#include <functional>
#include <vector>
#include <getopt.h>

struct Command {
	char shortName;
	std::string longName;
	bool requires_argument;
	std::function<bool(const std::string&)> handler;
	std::string help;
};

bool createQueue(const std::string& arg)
{
	return false;
}

bool showQueue(const std::string& arg)
{
	return false;
}

bool printHelp(const std::string& = "")
{
	printf(
		" usage: desaster-cli [options...]\n"
		"\n"
		"  -h,--hostname=VALUE                [localhost]\n"
		"  -p,--port=VALUE                    [2691]\n"
		"\n"
		"  --queue-create=NAME[,RATE[,CEIL]]  \n"
		"  --queue-list                       \n"
		"  --queue-rename=FROM,TO             \n"
		"  --queue-destroy=NAME,ALT           \n"
		"  --queue-adjust=NAME,RATE[,CEIL]    \n"
		"\n"
		"  --job-create=QUEUE,PAYLOAD         Enqueues a job into given queue.\n"
		"  --job-cancel=ID                    Cancels a pending job.\n"
		"  --job-move=[ID,]FROM,TO            Moves a job from a queue to another.\n"
		"                                     If no job ID was passed, all jobs\n"
		"                                     in given queue are moved.\n"
		"  --job-requeue=ID                   Puts this job to the end of its queue.\n"
		"  --job-prefer=ID                    Puts this job to the front of its queue.\n"
		"\n"
		"  --status                           \n"
		"\n"
	);

	return true;
}

std::vector<Command> commands = {
	{ 'h', "help", false, &printHelp, "Prints this help." },
	{ 0, "queue-create", true, &createQueue, "Creates a queue." },
	{ 0, "queue-show", true, &createQueue, "Shows a queue." }
};

int main(int argc, const char* argv[])
{
	printHelp();
	return 0;
}
