#ifndef desaster_NetworkAdapter_h
#define desaster_NetworkAdapter_h

#include <desaster/Module.h>
#include <desaster/NetMessage.h>
#include <desaster/Buffer.h>

#include <string>
#include <sstream>
#include <list>
#include <unordered_map>
#include <ev++.h>

/*! Implements networking interface to Desaster.
 */
class NetworkAdapter :
	public Module
{
public:
	class Connection;

private:
	int backlog_;
	int port_;
	std::string bindAddress_;

	ev::io listenWatcher_;
	std::list<Connection*> connections_;

	std::unordered_map<std::string, void (NetworkAdapter::*)(Connection* node, const char* args)> commands_;

	friend class Connection;

public:
	explicit NetworkAdapter(Server* server);
	~NetworkAdapter();

	int backlog() const { return backlog_; }
	int port() const { return port_; }
	const std::string& bindAddress() const { return bindAddress_; }

	bool configure(int port, const std::string& bindAddress);

	bool start();
	void stop();

	size_t size() const { return connections_.size(); }

private:
	void incoming(ev::io& io, int revents);
	Connection* unlink(Connection* connection);

	void handleCommand(Connection* con, const char* cmd, const char* arg);

	void createQueue(Connection* c, const char* arg);
	void listQueues(Connection* c, const char* arg);
	void showQueue(Connection* c, const char* arg);
	void destroyQueue(Connection* c, const char* arg);

	void createJob(Connection* c, const char* arg);
};

/*!
 * \brief represents TCP/IP connection from remote cluster node (or any client) to local cluster node.
 *
 * Processes client commands.
 * Client commands are text based, a request is a command token,
 * followed by a single space, and an variable length argument,
 * terminated by an LF.
 */
class NetworkAdapter::Connection { // {{{
private:
	NetworkAdapter* adapter_;
	int fd_;
	ev::io watcher_;
	ev::timer timeout_;
	unsigned long long messageCount_;

	Buffer readBuffer_;
	size_t readPos_;
	NetMessageParser parser_;

	Buffer writeBuffer_;
	size_t writePos_;
	NetMessageWriter writer_;

public:
	Connection(NetworkAdapter * server, int fd);
	Connection(const Connection&) = delete;
	Connection& operator=(const Connection&) = delete;
	~Connection();

	inline void writeArrayHeader(size_t arraySize);
	template<typename... Args> void writeMessage(Args... args);
	template<typename Arg> void writeValue(Arg arg);
	void writeError(const char* fmt, ...);
	void writeStatus(const char* fmt, ...);

private:
	void io(ev::io& io, int revents);
	void timeout(ev::timer& timer, int revents);

	void startRead();
	bool handleRead();

	void startWrite();
	bool handleWrite();

	void handleCommand();
}; // }}}

// {{{ inlines
inline void NetworkAdapter::Connection::writeArrayHeader(size_t arraySize)
{
	NetMessageWriter::writeArrayHeader(writeBuffer_, arraySize);
	startWrite();
}

template<typename... Args>
inline void NetworkAdapter::Connection::writeMessage(Args... args)
{
	NetMessageWriter::write(writeBuffer_, args...);
	startWrite();
}

template<typename Arg>
inline void NetworkAdapter::Connection::writeValue(Arg arg)
{
	NetMessageWriter::writeValue(writeBuffer_, arg);
	startWrite();
}
// }}}

#endif
