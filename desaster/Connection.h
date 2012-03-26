#ifndef desaster_Connection_h
#define desaster_Connection_h (1)

#include <ev++.h>

class Connection
{
private:
	int fd_;
	ev::io ioWatcher_;
	ev::timer ioTimeout_;
	int messageCount_;

public:
	class Message {
	private:
		std::string arg_;
	public:
		Message(Connection* owner);

		const std::strin& str() const { return arg_; }
		bool empty() const { return arg_.empty(); }
	};

public:
	Connection(Server* server, int fd);
	~Connection();

	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;

private:
	void io(ev::io&, int);
};

#endif
