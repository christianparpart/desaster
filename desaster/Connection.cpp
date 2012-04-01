#include <desaster/Connection.h>
#include <desaster/Server.h>
#include <cstring>

Connection::Connection(Server* server, int fd) :
	server_(server),
	fd_(fd),
	watcher_(server_->loop()),
	timeout_(server_->loop()),
	messageCount_(0),
	writeBuffer_(),
	writePos_(0)
{
	watcher_.set<Connection, &Connection::io>(this);
	timeout_.set<Connection, &Connection::timeout>(this);

	startRead();
}

void Connection::startRead()
{
	watcher_.set(fd_, ev::READ);
	watcher_.start();

	if (timeout_.is_active())
		timeout_.stop();

	timeout_.start(30, 0);
}

void Connection::startWrite()
{
	printf("startWrite\n");

	if (watcher_.is_active())
		watcher_.stop();
	watcher_.set(fd_, ev::WRITE);
	watcher_.start();

	if (timeout_.is_active())
		timeout_.stop();

	timeout_.start(30, 0);
}

Connection::~Connection()
{
	if (fd_ >= 0)
		::close(fd_);

	server_->unlink(this);
}

void Connection::io(ev::io&, int revents)
{
	printf("con.io: 0x%04x\n", revents);
	timeout_.stop();

	if (revents & ev::READ)
		if (!handleRead())
			return;

	if (revents & ev::WRITE)
		if (!handleWrite())
			return;

	timeout_.start(30, 0);
}

bool Connection::handleRead()
{
	char cmd[4096];
	ssize_t rv = ::read(fd_, cmd, sizeof(cmd));

	if (rv < 0) {
		perror("read");
		delete this;
		return false;
	} else if (rv == 0) {
		std::printf("client disconnected.\n");
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

		handleCommand(cmd, arg);

		return true;
	}
}

void Connection::handleCommand(const char* cmd, const char* arg)
{
	printf("handleCmd: '%s': '%s'\n", cmd, arg);

	auto i = server_->commands_.find(cmd);
	if (i != server_->commands_.end()) {
		(server_->*(i->second))(this, arg);
	} else {
		write("-ERR Unknown command\n");
	}
}

void Connection::write(const char* message)
{
	writeBuffer_ << message;
	startWrite();
}

bool Connection::handleWrite()
{
	printf("handle Write\n");
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

void Connection::timeout(ev::timer&, int revents)
{
	std::printf("Connection timed out.\n");
	delete this;
}
