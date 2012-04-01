#include <desaster/NetworkAdapter.h>
#include <desaster/Server.h>

#include <iostream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// {{{ NetworkAdapter
NetworkAdapter::NetworkAdapter(Server* server) :
	Module(server, "NetworkAdapter"),
	port_(2691),
	bindAddress_("0.0.0.0"),
	listenWatcher_(server->loop()),
	connections_(),
	commands_()
{
	commands_["queue-create"] = &NetworkAdapter::createQueue;
	commands_["queue-list"] = &NetworkAdapter::listQueues;
	commands_["queue-show"] = &NetworkAdapter::showQueue;
	commands_["queue-destroy"] = &NetworkAdapter::destroyQueue;
}

NetworkAdapter::~NetworkAdapter()
{
}

bool NetworkAdapter::configure(int port, const std::string& bindAddress)
{
	port_ = port;
	bindAddress_ = bindAddress;

	return true;
}

bool NetworkAdapter::start()
{
	notice("Setting up listener at tcp://%s:%d", bindAddress_.c_str(), port_);

	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return false;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port_);

	int rv;
	if ((rv = inet_pton(AF_INET, bindAddress_.c_str(), &sin.sin_addr.s_addr)) <= 0) {
		if (rv == 0)
			error("Address not in representation format.");
		else
			perror("inet_pton");
		close(fd);
		return false;
	}

	rv = bind(fd, (sockaddr*)&sin, sizeof(sin));
	if (rv < 0) {
		perror("bind");
		close(fd);
		return false;
	}

	rv = listen(fd, backlog_);
	if (rv < 0) {
		perror("listen");
		close(fd);
		return false;
	}

	listenWatcher_.set<NetworkAdapter, &NetworkAdapter::incoming>(this);
	listenWatcher_.set(fd, ev::READ);
	listenWatcher_.start();

	return true;
}

void NetworkAdapter::incoming(ev::io& listener, int revents)
{
	debug("Server: new client connected");

	sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	int fd = accept(listener.fd, (sockaddr*)&sin, &slen);

	if (Connection* c = new Connection(this, fd)) {
		connections_.push_back(c);
	} else {
		close(fd);
	}
}

NetworkAdapter::Connection* NetworkAdapter::unlink(Connection* connection)
{
	auto i = std::find(connections_.begin(), connections_.end(), connection);
	if (i != connections_.end())
		connections_.erase(i);

	return connection;
}

void NetworkAdapter::stop()
{
	if (listenWatcher_.is_active()) {
		listenWatcher_.stop();
		::close(listenWatcher_.fd);
	}
}

void NetworkAdapter::handleCommand(Connection* con, const char* cmd, const char* arg)
{
	debug("handleCmd: '%s': '%s'", cmd, arg);

	auto i = commands_.find(cmd);
	if (i != commands_.end()) {
		(this->*(i->second))(con, arg);
	} else {
		con->write("-ERR Unknown command\n");
	}
}

void NetworkAdapter::createQueue(Connection* c, const char* arg)
{
	debug("creating queue");
}

void NetworkAdapter::listQueues(Connection* c, const char* arg)
{
	debug("list queues");
}

void NetworkAdapter::showQueue(Connection* c, const char* arg)
{
	debug("show queue");
}

void NetworkAdapter::destroyQueue(Connection* c, const char* arg)
{
	debug("show queue");
}
// }}}
// {{{ NetworkAdapter::Connection
NetworkAdapter::Connection::Connection(NetworkAdapter* adapter, int fd) :
	adapter_(adapter),
	fd_(fd),
	watcher_(adapter->server().loop()),
	timeout_(adapter->server().loop()),
	messageCount_(0),
	writeBuffer_(),
	writePos_(0)
{
	watcher_.set<Connection, &Connection::io>(this);
	timeout_.set<Connection, &Connection::timeout>(this);

	startRead();
}

void NetworkAdapter::Connection::startRead()
{
	watcher_.set(fd_, ev::READ);
	watcher_.start();

	if (timeout_.is_active())
		timeout_.stop();

	timeout_.start(30, 0);
}

void NetworkAdapter::Connection::startWrite()
{
	if (watcher_.is_active())
		watcher_.stop();
	watcher_.set(fd_, ev::WRITE);
	watcher_.start();

	if (timeout_.is_active())
		timeout_.stop();

	timeout_.start(30, 0);
}

NetworkAdapter::Connection::~Connection()
{
	if (fd_ >= 0)
		::close(fd_);

	adapter_->unlink(this);
}

void NetworkAdapter::Connection::io(ev::io&, int revents)
{
	timeout_.stop();

	if (revents & ev::READ)
		if (!handleRead())
			return;

	if (revents & ev::WRITE)
		if (!handleWrite())
			return;

	timeout_.start(30, 0);
}

bool NetworkAdapter::Connection::handleRead()
{
	char cmd[4096];
	ssize_t rv = ::read(fd_, cmd, sizeof(cmd));

	if (rv < 0) {
		perror("read");
		delete this;
		return false;
	} else if (rv == 0) {
		adapter_->debug("client disconnected.");
		delete this;
		return false;
	} else {
		// strip right
		while (rv > 0 && std::isspace(cmd[rv - 1]))
			--rv;
		cmd[rv] = '\0';

		// split args off the command 
		char *arg = std::strchr(cmd, ' ');
		if (arg) {
			*arg = '\0';
			++arg;
		} else
			arg = cmd + rv;

		adapter_->handleCommand(this, cmd, arg);

		return true;
	}
}

void NetworkAdapter::Connection::write(const char* message)
{
	writeBuffer_ << message;
	startWrite();
}

bool NetworkAdapter::Connection::handleWrite()
{
	std::stringbuf* buf = writeBuffer_.rdbuf();
	const char* p = &buf->str()[writePos_];

	ssize_t rv = ::write(fd_, p, writeBuffer_.tellp() - writePos_);
	if (rv < 0) {
		perror("write");
		return false;
	}

	writePos_ += rv;

	if (writePos_ == writeBuffer_.tellp()) {
		writeBuffer_.seekp(0);
		writePos_ = 0;
		startRead();
	}
	return true;
}

void NetworkAdapter::Connection::timeout(ev::timer&, int revents)
{
	adapter_->info("Connection timed out.");
	delete this;
}
// }}}
