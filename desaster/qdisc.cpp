#include "qdisc.h"
#include <cassert>
#include <climits>
#include <algorithm>

namespace qdisc {

// {{{ bucket
bucket::bucket(size_t capacity, const std::string& name) :
	name_(name),
	capacity_(capacity),
	available_(capacity)
{
}

bucket::~bucket()
{
}

std::string bucket::str() const
{
	char buf[256];
	snprintf(buf, sizeof(buf), "bucket[%s] %ld/%ld", name_.c_str(), size(), capacity());
	return buf;
}
// }}}
// {{{ fifo
fifo::fifo(size_t capacity, const std::string& name) :
	bucket(capacity, name)
{
}

size_t fifo::get(size_t n)
{
	if (n > available_)
		return 0;

	available_ -= n;
	return n;
}

void fifo::put(size_t n)
{
	assert(available_ + n <= capacity_);
	available_ += n;
}

void fifo::accept(visitor* c)
{
	c->accept(this);
}
// }}}
// {{{ classful
classful::classful(size_t capacity, const std::string& name) :
	bucket(capacity, name)
{
}

classful::~classful()
{
	for (auto bucket: classes_) {
		delete bucket;
	}
}
// }}}
// {{{ htb
// {{{ htb::node
htb::node::node(const std::string& name, bucket* parent,
		size_t rate, size_t ceil,
		size_t burst, size_t quantum) :
	bucket(ceil ? ceil : rate, name),
	parent_(parent),
	rate_(rate),
	burst_(burst),
	quantum_(quantum)
{
}

htb::node::~node()
{
}

size_t htb::node::get(size_t n)
{
	if (actual_rate() + n > ceil())
		return 0;

	if (actual_rate() + n <= rate()) {
		n = std::min(burst_, n);
		size_t result = parent_->get(n);
		available_ -= result;
		return result;
	} else if (parent_) {
		// XXX here we might be more inteligent to drain n'
		n = std::min(quantum_, n);
		size_t result = parent_->get(n);
		available_ -= result;
		return result;
	}

	return 0;
}

void htb::node::put(size_t n)
{
	assert(actual_rate() + n <= ceil());

	size_t overrate = over_rate();
	if (overrate) {
		size_t red = std::min(overrate, n);
		size_t green = n - red;
		parent_->put(red);
		available_ += green;
	} else {
		available_ += n;
	}
}

void htb::node::accept(visitor* c)
{
	c->accept(this);
}

std::string htb::node::str() const
{
	char buf[256];
	snprintf(buf, sizeof(buf), "htb::node[%s] AR:%ld, R:%ld, C:%ld, B:%ld, Q:%ld",
		name_.c_str(), actual_rate(), rate(), ceil(), burst(), quantum());
	return buf;
}
// }}}
// {{{ htb
htb::htb(size_t capacity, const std::string& name) :
	classful(capacity, name)
{
}

htb::~htb()
{
}

htb::node* htb::create_child(
	const std::string& name,
	size_t rate, size_t ceil,
	size_t burst, size_t quantum)
{
	auto child = new node(name, this, rate, ceil, burst, quantum);

	classes_.push_back(child);

	return child;
}

htb::node* htb::find(const std::string& name)
{
	// TODO -> unordered_map
	for (auto n: classes_)
		if (n->name() == name)
			return static_cast<node*>(n);

	return nullptr;
}

size_t htb::get(size_t n)
{
	if (n > available_)
		return 0;

	available_ -= n;
	return n;
}

void htb::put(size_t n)
{
	assert(available_ + n <= capacity_);
	available_ += n;
}

void htb::accept(visitor* c)
{
	c->accept(this);
}

std::string htb::str() const
{
	char buf[256];
	snprintf(buf, sizeof(buf), "htb[%s] AR:%ld C:%ld", name_.c_str(), size(), capacity());
	return buf;
}
// }}}
// }}}
} // namespace qdisc
