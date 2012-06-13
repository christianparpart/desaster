#include <desaster/NetworkAdapter.h>
#include <desaster/NetMessage.h>
#include <desaster/Server.h>
#include <desaster/Module.h>
#include <desaster/Queue.h>
#include <desaster/Job.h>

#include <iostream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

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

	int rc = 1;
#if defined(SO_REUSEADDR)
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof(rc)) < 0) {
		perror("setsockopt");
		return false;
	}
#endif

#if defined(TCP_QUICKACK)
	if (::setsockopt(fd, SOL_TCP, TCP_QUICKACK, &rc, sizeof(rc)) < 0) {
		perror("setsockopt");
		return false;
	}
#endif

#if defined(TCP_DEFER_ACCEPT) && defined(WITH_TCP_DEFER_ACCEPT)
	if (::setsockopt(fd, SOL_TCP, TCP_DEFER_ACCEPT, &rc, sizeof(rc)) < 0) {
		perror("setsockopt");
		return false;
	}
#endif

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port_);

	if ((rc = inet_pton(AF_INET, bindAddress_.c_str(), &sin.sin_addr.s_addr)) <= 0) {
		if (rc == 0)
			error("Address not in representation format.");
		else
			perror("inet_pton");
		close(fd);
		return false;
	}

	rc = bind(fd, (sockaddr*)&sin, sizeof(sin));
	if (rc < 0) {
		perror("bind");
		close(fd);
		return false;
	}

	rc = listen(fd, backlog_);
	if (rc < 0) {
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
		con->writeError("Unknown command");
	}
}

void NetworkAdapter::createQueue(Connection* c, const char* arg)
{
	debug("creating queue");
	c->writeStatus("OK");
}

void NetworkAdapter::listQueues(Connection* c, const char* arg)
{
	c->writeArrayHeader(server().queues().size());
	for (auto queue: server().queues()) {
		c->writeMessage(
			queue->name().c_str(),
			queue->actualRate(),
			queue->rate(),
			queue->ceil(),
			queue->size()
		);
	}
}

void NetworkAdapter::showQueue(Connection* c, const char* arg)
{
	Queue* queue = server().findQueue(arg);
	if (!queue) {
		c->writeError("Queue not found");
		return;
	}

	c->writeMessage(
		queue->actualRate(),
		queue->rate(),
		queue->ceil(),
		queue->size()
	);
}

void NetworkAdapter::destroyQueue(Connection* c, const char* arg)
{
	debug("destroy queue");
	c->writeError("not implemented yet");
}

void NetworkAdapter::createJob(Connection* c, const char* arg)
{
	c->writeError("not implemented yet");
}
// }}}
// {{{ NetworkAdapter::Connection
NetworkAdapter::Connection::Connection(NetworkAdapter* adapter, int fd) :
	adapter_(adapter),
	socket_(new NetMessageSocket(adapter->server().loop(), fd, true)),
	messageCount_(0)
{
	socket_->setReceiveHook(std::bind(&NetworkAdapter::Connection::onMessage, this, std::placeholders::_1));
	//socket_->setReceiveHook([&](NetMessageSocket*) { onMessage(0); });
}

NetworkAdapter::Connection::~Connection()
{
	delete socket_;
	adapter_->unlink(this);
}

void NetworkAdapter::Connection::onMessage(NetMessageSocket* socket)
{
	NetMessage* message = socket_->message();

	if (!message->isArray()) {
		writeError("Wrong message type");
		return;
	}

	size_t n = message->size();
	std::vector<const char*> args(n - 1);
	for (size_t i = 0; i < args.size(); ++i) {
		const NetMessage& arg = message->toArray()[i + 1];
		if (arg.isString())
			args[i] = arg.toString();
		else if (arg.isNil())
			args[i] = nullptr;
		else {
			socket_->writeError("Wrong message type");
			return;
		}
	}
}

void NetworkAdapter::Connection::onTimeout(NetMessageSocket* socket)
{
	adapter_->info("Connection timed out.");
	delete this;
}

void NetworkAdapter::Connection::writeStatus(const char* fmt, ...)
{
	va_list va;
	char buf[4096];

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);

	socket_->writeStatus(buf);
}

void NetworkAdapter::Connection::writeError(const char* fmt, ...)
{
	va_list va;
	char buf[4096];

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);

	socket_->writeError(buf);
}
// }}}
