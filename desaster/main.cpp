#include "qdisc.h"
#include "Server.h"
#include <ev++.h>
#include <cstdio>
#include <sstream>
#include <memory>

void dump(qdisc::classful* disc, const char* msg)
{
	printf("%s -- %s\n", disc->str().c_str(), msg);
	std::stringstream sstr;
	size_t i = 0;
	for (auto sub: disc->classes()) {
		printf(" %ld:  %s\n", i, sub->str().c_str());
		for (size_t k = 0; k < sub->size(); ++k) {
			sstr << char('0' + i);
		}
		++i;
	}

	for (size_t k = 0; k < disc->available(); ++k)
		sstr << '-';

	printf(" distribution: [%s]\n", sstr.str().c_str());
}

class Job : public qdisc::visitor
{
private:
	std::string tag_;
	std::string command_;
	qdisc::bucket* owner_;

public:
	Job(const std::string& tag, const std::string& command) :
		tag_(tag), command_(command), owner_(nullptr) {}
	~Job() {}

	virtual void accept(qdisc::fifo* n) {
		if (n->get(1)) {
			// pass job to a worker
			owner_ = n;
		} else {
			// queue job
			//n->enqueue(job);
		}
	}

	virtual void accept(qdisc::htb::node* n) {
		if (n->get(1)) {
			// pass job to worker
			owner_ = n;
		} else {
			//c->enqueue(job);
		}
	}

	virtual void accept(qdisc::htb* n) {
		auto c = n->find(tag_);

		if (c) {
			accept(c);
		} else {
			// there is no queue with that name -> enqueue to default queue
		}
	}
};

void qdisc_test()
{
	auto root = std::make_shared<qdisc::htb>(10, "root");
	root->create_child("n0", 3, 3, 2, 1);
	root->create_child("n1", 2, 5, 2, 1);
	root->create_child("n2", 5, 7, 5, 1);
	dump(root.get(), "after create");

	root->accept(new Job("n1", "uptime"));
	dump(root.get(), "j1");

	root->accept(new Job("n1", "ls"));
	dump(root.get(), "j2");

	root->accept(new Job("n1", "find /"));
	dump(root.get(), "j3");

	root->accept(new Job("n1", "acpitool"));
	dump(root.get(), "j4");

	for (int i = 0; i < 5; ++i) {
		root->accept(new Job("n0", "find"));
		dump(root.get(), "n0-job");
	}
}

int main(int argc, char* argv[])
{
	ev::default_loop loop;
	Server server(loop);

	if (!server.setup(argc, argv))
		return 1;

	qdisc_test();

	//loop.run();

	return 0;
}
