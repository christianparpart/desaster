#ifndef desaster_Connection_h
#define desaster_Connection_h (1)

#include <ev++.h>
#include <sstream>

class Server;

/*!
 * \brief represents TCP/IP connection from remote cluster node (or any client) to local cluster node.
 *
 * Processes client commands
 */
class Connection
{
public:
	class Message { // {{{
	private:
		Connection* connection_;
		unsigned long long id_;
		std::string command_;
		std::string arg_;

	public:
		Message(Connection* connection, unsigned long long id, const std::string& cmd, const std::string& arg) :
			connection_(connection),
			id_(id),
			command_(cmd),
			arg_(arg)
		{}

		Message(Message&& other) :
			connection_(std::move(other.connection_)),
			id_(std::move(other.id_)),
			command_(std::move(other.command_)),
			arg_(std::move(other.arg_))
		{}

		Message&& operator=(Message&& other) {
			connection_ = std::move(other.connection_);
			id_ = std::move(other.id_);
			command_ = std::move(other.command_);
			arg_ = std::move(other.arg_);

			return std::move(*this);
		}

		Message(const Message&) = delete;
		Message& operator=(const Message&) = delete;

		const std::string& command() const { return command_; }
		const std::string& argument() const { return arg_; }
	}; // }}}

private:
	Server* server_;
	int fd_;
	ev::io watcher_;
	ev::timer timeout_;
	unsigned long long messageCount_;

	std::stringstream writeBuffer_;
	ssize_t writePos_;

public:
	Connection(Server* server, int fd);
	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;
	~Connection();

	void write(const char* message);

private:
	void io(ev::io& io, int revents);
	void timeout(ev::timer& timer, int revents);

	void startRead();
	bool handleRead();
	void handleCommand(const char* cmd, const char* arg);
	void startWrite();
	bool handleWrite();
};

#endif
