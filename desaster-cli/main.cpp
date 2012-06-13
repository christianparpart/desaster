#include <desaster/NetMessage.h>
#include <desaster/config.h>
#include <desaster/CLI.h>
#include <string>
#include <getopt.h>

using desaster::CLI;

static std::string hostname = "localhsot";
static int port = 2691;

int printHelp(const CLI::Params& args = CLI::Params()) // {{{
{
	const char* homepageUrl = "http://github.com/trapni/desaster/";

	std::printf(
		"desaster-cli, Job Queueing Manager Client, version %s [%s]\n"
		"\n"
		"Copyright (c) 2012 by Christian Parpart <trapni@gentoo.org>\n"
		"Licensed under GPL-3 [http://gplv3.fsf.org/]\n"
		"\n"
		"  usage: desaster-cli [options...] command ...\n"
		"\n"
		"  -h,--hostname=VALUE              [%s]\n"
		"  -p,--port=VALUE                  [%d]\n"
		"\n"
		"  queue create NAME [RATE [CEIL]]  Creates a new job queue.\n"
		"  queue index                      Lists all available job queues.\n"
		"  queue show NAME                  Shows a given job queue in detail.\n"
		"  queue rename FROM TO             Renames a job queue.\n"
		"  queue destroy NAME [ALT]         Destroys a job queue, possibly requeuing\n"
		"                                   existing jobs onto an alternative queue.\n"
		"  queue adjust NAME RATE [CEIL]    Tweaks rate and ceil values of a queue.\n"
		"\n"
		"  job create QUEUE PAYLOAD         Enqueues a job onto given queue.\n"
		"  job show ID                      Shows job details.\n"
		"  job move [ID] FROM TO            Requeues (moves) a job from a queue to another.\n"
		"                                   If no job ID was passed, all jobs\n"
		"                                   in given queue are moved.\n"
		"  job requeue ID                   Puts this job to the end of its queue.\n"
		"  job prefer ID                    Puts this job to the front of its queue.\n"
		"  job cancel ID                    Cancels a pending job.\n"
		"\n"
		"  status                           Prints general status informations.\n"
		"\n",
		PACKAGE_VERSION,
		homepageUrl,
		hostname.c_str(),
		port
	);

	return false;
} // }}}

// {{{ queue actions
int createQueue(const CLI::Params& args)
{
	ev::default_loop loop;
	NetMessageSocket stream(loop, hostname, port);

	stream.setReceiveHook([&](NetMessageSocket*) {
		printf("message received\n");
		stream.close();
	});

	stream.writeMessage("queue-create", args[0]);

	loop.run();

	return 0;
}

int indexQueues(const CLI::Params& args)
{
	return 0;
}

int showQueue(const CLI::Params& args)
{
	return 0;
}

int renameQueue(const CLI::Params& args)
{
	return 0;
}

int adjustQueue(const CLI::Params& args)
{
	return 0;
}

int destroyQueue(const CLI::Params& args)
{
	return 0;
}
// }}}

// {{{ job actions
bool createJob(const std::string& arg)
{
	return false;
}

bool showJob(const std::string& arg)
{
	return false;
}

bool moveJob(const std::string& arg)
{
	return false;
}

bool requeueJob(const std::string& arg)
{
	return false;
}

bool preferJob(const std::string& arg)
{
	return false;
}

bool cancelJob(const std::string& arg)
{
	return false;
}
// }}}

// {{{ misc actions
bool serviceStatus()
{
	return false;
}
// }}}

int main(int argc, char* argv[]) {
	desaster::CLI cli;

	cli.create().
		group().
			command("help", &printHelp).
			command("status").
		group("queue").
			command("create", &createQueue).
			command("index", &indexQueues).
			command("show", &showQueue).
			command("rename", &renameQueue).
			command("adjust", &adjustQueue).
			command("destroy", &destroyQueue).
		group("job").
			command("enqueue").//, &createJob, "ID", "Creates a job").
			command("enqueue-wait").//, &createJob, "ID", "Creates a job").
			command("wait").//, &createJob, "ID", "Creates a job").
			command("info").
			command("output").
			command("move").
			command("requeue").
			command("prefer").
			command("cancel");

	try {
		cli.setupShell(std::string(getenv("HOME")) + "/.desaster-cli_history");
		return argc != 1
			? cli.evaluate(argc - 1, argv + 1)
			: cli.shell();
	} catch (CLI::NotFoundError& e) {
		fprintf(stderr, "%s\n", e.what());
		return 1;
	} catch (CLI::MultipleCommandsMatchError& e) {
		fprintf(stderr, "%s\n", e.what());
		return 2;
	}
}
