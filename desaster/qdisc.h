#ifndef qdisc_h
#define qdisc_h (1)

#include <cstddef>
#include <vector>
#include <string>

namespace qdisc {

class visitor;

class bucket
{
protected:
	std::string name_;
	size_t capacity_;
	size_t available_;

public:
	explicit bucket(size_t capacity, const std::string& name = "");
	virtual ~bucket();

	const std::string& name() const { return name_; }

	size_t capacity() const { return capacity_; }
	size_t available() const { return available_; }
	size_t size() const { return capacity_ - available_; }

	virtual bool reserve(size_t capacity);

	virtual size_t get(size_t n) = 0;
	virtual void put(size_t n) = 0;

	virtual void accept(visitor* c) = 0;

	virtual std::string str() const;
};

class fifo : public bucket
{
public:
	explicit fifo(size_t capacity, const std::string& name = "");

	virtual size_t get(size_t n);
	virtual void put(size_t n);
	virtual void accept(visitor* c);
};

class classful : public bucket
{
protected:
	std::vector<bucket*> classes_;

public:
	classful(size_t capacity, const std::string& name = "");
	~classful();

	std::vector<bucket*>& classes() { return classes_; }
};

class htb : public classful
{
public:
	class node : public bucket { // {{{
	private:
		bucket* parent_;
		size_t rate_;
		size_t burst_;
		size_t quantum_;

	public:
		node(const std::string& name, bucket* parent,
			size_t rate, size_t ceil,
			size_t burst, size_t quantum);
		~node();

		const std::string& name() const { return name_; }
		bucket* parent() const { return parent_; }
		size_t rate() const { return rate_; }
		size_t actual_rate() const { return size(); }
		size_t over_rate() const { return actual_rate() > rate() ? actual_rate() - rate() : 0; }
		size_t ceil() const { return capacity(); }
		size_t burst() const { return burst_; }
		size_t quantum() const { return quantum_; }

		virtual size_t get(size_t n);
		virtual void put(size_t n);
		virtual void accept(visitor* c);

		virtual std::string str() const;
	}; // }}}

public:
	explicit htb(size_t capacity, const std::string& name = "");
	~htb();

	node* create_child(const std::string& name,
		size_t rate = 0, size_t ceil = 0,
		size_t burst = 1, size_t quantum = 1);

	void destroy_child(const std::string& name);

	node* find(const std::string& name);

	virtual size_t get(size_t n);
	virtual void put(size_t n);
	virtual void accept(visitor* c);

	virtual std::string str() const;
};

class visitor
{
public:
	virtual ~visitor() {}

	virtual void accept(fifo* bucket) = 0;
	//virtual void accept(prio* bucket) = 0;
	virtual void accept(htb* bucket) = 0;
	virtual void accept(htb::node*) = 0;
};

} // namespace qdisc

#endif
