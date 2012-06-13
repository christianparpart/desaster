#include <desaster/NetMessage.h>
#include <memory>
#include <cctype>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define TRACE(msg...) { printf("NetMessage: " msg); printf("\n"); }
//#define TRACE(msg...) /*!*/ ((void)0)

// {{{ NetMessage
NetMessage::NetMessage() :
	type_(Nil),
	number_()
{
}

NetMessage::NetMessage(Type t, char* buf, size_t size) :
	type_(t),
	number_(size),
	string_(buf)
{
}

NetMessage::NetMessage(Type t, long long value) :
	type_(t),
	number_(value)
{
	if (type_ == Array) {
		array_ = new NetMessage[value];
	}
}

NetMessage::~NetMessage()
{
	clear();
}

void NetMessage::clear()
{
	switch (type_) {
		case Status:
		case Error:
		case String:
			delete[] string_;
			break;
		case Array:
			delete[] array_;
			break;
		default:
			break;
	}
}

NetMessage* NetMessage::createNil()
{
	return new NetMessage();
}

NetMessage* NetMessage::createStatus(const BufferRef& message)
{
	size_t size = message.size();
	char* buf = new char[size + 1];
	memcpy(buf, message.data(), size);
	buf[size] = 0;

	return new NetMessage(Status, buf, size);
}

NetMessage* NetMessage::createError(const BufferRef& message)
{
	size_t size = message.size();
	char* buf = new char[size + 1];
	memcpy(buf, message.data(), size);
	buf[size] = 0;

	return new NetMessage(Error, buf, size);
}

NetMessage* NetMessage::createNumber(long long value)
{
	return new NetMessage(Number, value);
}

NetMessage* NetMessage::createString(const BufferRef& value)
{
	size_t size = value.size();
	char* buf = new char[size + 1];
	memcpy(buf, value.data(), size);
	buf[size] = 0;

	return new NetMessage(String, buf, size);
}

NetMessage* NetMessage::createArray(size_t size)
{
	return new NetMessage(Array, size);
}

void NetMessage::setNil()
{
	clear();
}

void NetMessage::setNumber(long long value)
{
	clear();

	type_ = Number;
	number_ = value;
}

void NetMessage::setString(const char* value, size_t size)
{
	clear();

	type_ = String;
	string_ = new char[size + 1];
	memcpy(string_, value, size);
	string_[size] = '\0';
}

void NetMessage::setArray(size_t size)
{
	clear();

	type_ = Array;
	number_ = size;

	if (size)
		array_ = new NetMessage[size];
	else
		array_ = nullptr;
}
// }}}
// {{{ NetMessageReader
const char* NetMessageReader::state_str() const
{
	switch (state()) {
	case MESSAGE_BEGIN:
		return "message-begin";
	case MESSAGE_TYPE:
		return "message-type";
	case MESSAGE_LINE_BEGIN:
		return "message-line-begin";
	case MESSAGE_LINE_OR_CR:
		return "message-line-or-cr";
	case MESSAGE_LINE_LF:
		return "message-line-lf";
	case MESSAGE_NUM_ARGS:
		return "message-num-args";
	case MESSAGE_NUM_ARGS_OR_CR:
		return "message-num-args-or-cr";
	case MESSAGE_LF:
		return "message-lf";
	case BULK_BEGIN:
		return "bulk-begin";
	case BULK_SIZE:
		return "bulk-size";
	case BULK_SIZE_OR_CR:
		return "bulk-size-or-cr";
	case BULK_SIZE_LF:
		return "bulk-size-lf";
	case BULK_BODY_OR_CR:
		return "bulk-body-or-cr";
	case BULK_BODY_LF:
		return "bulk-body-lf";
	case MESSAGE_END:
		return "message-end";
	case SYNTAX_ERROR:
		return "syntax-error";
	default:
		return "unknown";
	}
}

NetMessageReader::NetMessageReader(const Buffer* buf) :
	buffer_(buf),
	pos_(0),
	currentContext_(nullptr),
	begin_(0),
	argSize_(0)
{
	pushContext(); // create root context
}

NetMessageReader::~NetMessageReader()
{
}

void NetMessageReader::parse()
{
	while (!isEndOfBuffer()) {
		if (std::isprint(currentChar())) {
			TRACE("parse: '%c' (%d) %s", currentChar(), static_cast<int>(state()), state_str());
		} else {
			TRACE("parse: 0x%02X (%d) %s", currentChar(), static_cast<int>(state()), state_str());
		}

		switch (state()) {
			case MESSAGE_BEGIN:
				// Syntetic state. Go straight to TYPE.
			case MESSAGE_TYPE:
				switch (currentChar()) {
					case '+':
					case '-':
					case ':':
						currentContext_->type = static_cast<NetMessage::Type>(currentChar());
						setState(MESSAGE_LINE_BEGIN);
						nextChar();
						break;
					case '$':
						currentContext_->type = NetMessage::String;
						setState(BULK_BEGIN);
						break;
					case '*':
						currentContext_->type = NetMessage::Array;
						setState(MESSAGE_NUM_ARGS);
						nextChar();
						break;
					default:
						currentContext_->type = NetMessage::Nil;
						setState(SYNTAX_ERROR);
						return;
				}
				break;
			case MESSAGE_LINE_BEGIN:
				if (currentChar() == '\r') {
					setState(SYNTAX_ERROR);
					return;
				}
				setState(MESSAGE_LINE_OR_CR);
				begin_ = pos_;
				nextChar();
				break;
			case MESSAGE_LINE_OR_CR:
				if (currentChar() == '\n') {
					setState(SYNTAX_ERROR);
					return;
				}

				if (currentChar() == '\r')
					setState(MESSAGE_LINE_LF);

				nextChar();
				break;
			case MESSAGE_LINE_LF: {
				if (currentChar() != '\n') {
					setState(SYNTAX_ERROR);
					return;
				}
				BufferRef value = buffer_->ref(begin_, pos_ - begin_ - 1);
				switch (currentContext_->type) {
					case NetMessage::Status:
						currentContext_->message = NetMessage::createStatus(value);
						break;
					case NetMessage::Error:
						currentContext_->message = NetMessage::createError(value);
						break;
					case NetMessage::String:
						currentContext_->message = NetMessage::createString(value);
						break;
					case NetMessage::Number:
						currentContext_->message = NetMessage::createNumber(value.toInt());
						break;
					default:
						currentContext_->message = NetMessage::createNil();
						break;
				}
				setState(MESSAGE_END);
				nextChar();
				popContext();
				break;
			}
			case MESSAGE_NUM_ARGS: {
				switch (currentChar()) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						currentContext_->number *= 10;
						currentContext_->number += currentChar() - '0';
						setState(MESSAGE_NUM_ARGS_OR_CR);
						nextChar();
						break;
					default:
						setState(SYNTAX_ERROR);
						return;
				}
				break;
			}
			case MESSAGE_NUM_ARGS_OR_CR:
				switch (currentChar()) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						currentContext_->number *= 10;
						currentContext_->number += currentChar() - '0';
						nextChar();
						break;
					case '\r':
						setState(MESSAGE_LF);
						currentContext_->message = NetMessage::createArray(currentContext_->number);
						nextChar();
						break;
					default:
						setState(SYNTAX_ERROR);
						return;
				}
				break;
			case MESSAGE_LF:
				if (currentChar() != '\n') {
					setState(SYNTAX_ERROR);
					return;
				}

				nextChar();

				if (currentContext_->type == NetMessage::Array) {
					setState(BULK_BEGIN);
					pushContext();
				} else {
					setState(MESSAGE_END);
					popContext();
				}
				break;
			case BULK_BEGIN:
				if (currentChar() != '$') {
					setState(SYNTAX_ERROR);
					return;
				}
				setState(BULK_SIZE);
				nextChar();
				break;
			case BULK_SIZE:
				switch (currentChar()) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						argSize_ *= 10;
						argSize_ += currentChar() - '0';
						setState(BULK_SIZE_OR_CR);
						nextChar();
						break;
					default:
						setState(SYNTAX_ERROR);
						return;
				}
				break;
			case BULK_SIZE_OR_CR:
				switch (currentChar()) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						argSize_ *= 10;
						argSize_ += currentChar() - '0';
						nextChar();
						break;
					case '\r':
						setState(BULK_SIZE_LF);
						nextChar();
						break;
					default:
						setState(SYNTAX_ERROR);
						return;
				}
				break;
			case BULK_SIZE_LF:
				if (currentChar() != '\n') {
					setState(SYNTAX_ERROR);
					return;
				}
				nextChar();
				setState(BULK_BODY_OR_CR);
				begin_ = pos_;
				break;
			case BULK_BODY_OR_CR:
				if (argSize_ > 0) {
					argSize_ -= nextChar(argSize_);
				} else if (currentChar() == '\r') {
					BufferRef value = buffer_->ref(begin_, pos_ - begin_);
					currentContext_->message = NetMessage::createString(value);
					nextChar();
					setState(BULK_BODY_LF);
				} else {
					setState(SYNTAX_ERROR);
					return;
				}
				break;
			case BULK_BODY_LF:
				if (currentChar() != '\n') {
					setState(SYNTAX_ERROR);
					return;
				}
				nextChar();

				setState(MESSAGE_END);
				popContext();

				break;
			case MESSAGE_END:
				// if we reach here, then only because
				// there's garbage at the end of our message.
				break;
			case SYNTAX_ERROR:
				fprintf(stderr, "NetMessageSocket message syntax error at offset %zi\n", pos_);
				break;
			default:
				break;
		}
	}
}

inline void NetMessageReader::nextChar()
{
	if (!isEndOfBuffer()) {
		++pos_;
	}
}

inline size_t NetMessageReader::nextChar(size_t n)
{
	size_t avail = buffer_->size() - pos_;
	n = std::min(n, avail);
	pos_ += n;
	return n;
}

inline BufferRef NetMessageReader::currentValue() const
{
	return buffer_->ref(begin_, pos_ - begin_);
}

void NetMessageReader::pushContext()
{
	TRACE("pushContext:");
	ParseContext* pc = new ParseContext();
	pc->parent = currentContext_;
	currentContext_ = pc;

	setState(MESSAGE_BEGIN);
}

void NetMessageReader::popContext()
{
	TRACE("popContext:");
	ParseContext* pc = currentContext_;

	if (!pc->parent) {
		TRACE("popContext: do not pop. we're already at root.");
		return;
	}

	currentContext_ = currentContext_
		? currentContext_->parent
		: nullptr;

	pc->parent = nullptr;

	delete pc;
}
// }}}
// {{{ NetMessageSocket
NetMessageSocket::NetMessageSocket(ev::loop_ref loop, int fd, bool autoClose) :
	Logging("NetMessageSocket[fd:%d]", fd),
	loop_(loop),
	socketWatcher_(loop_),
	socketTimer_(loop),
	fd_(fd),
	autoClose_(autoClose),
	writeBuffer_(),
	writePos_(0),
	readBuffer_(),
	readPos_(0),
	reader_(&readBuffer_),
	receiveHook_()
{
	socketWatcher_.set<NetMessageSocket, &NetMessageSocket::io>(this);
	socketTimer_.set<NetMessageSocket, &NetMessageSocket::timeout>(this);

	startRead();
}

NetMessageSocket::NetMessageSocket(ev::loop_ref loop, const std::string& hostname, int port) :
	Logging("NetMessageSocket[%s:%d]", hostname.c_str(), port),
	loop_(loop),
	socketWatcher_(loop_),
	socketTimer_(loop),
	fd_(-1),
	autoClose_(true),
	writeBuffer_(),
	writePos_(0),
	readBuffer_(),
	readPos_(0),
	reader_(&readBuffer_),
	receiveHook_()
{
	socketWatcher_.set<NetMessageSocket, &NetMessageSocket::io>(this);
	socketTimer_.set<NetMessageSocket, &NetMessageSocket::timeout>(this);

	open(hostname, port);
}

NetMessageSocket::~NetMessageSocket()
{
	close();
}

/*! opens a connection to a remote server at given \p hostname and \p port number.
 *
 * \param hostname the hostname of the host to connect to.
 * \param port the TCP port number to connect to.
 *
 * \throw ConnectError if connection to given host failed.
 *
 * \todo asynchronous/nonblocking connect()
 * \todo DNS resolving
 */
void NetMessageSocket::open(const std::string& hostname, int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw ConnectError("socket", errno);

	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	// TODO support DNS resolving.
	int rc;
	if ((rc = inet_pton(AF_INET, hostname.c_str(), &sin.sin_addr.s_addr)) <= 0) {
		::close(fd);
		if (rc == 0)
			throw ConnectError("Address not in representation format.");
		else
			throw ConnectError("inet_pton", errno);
	}

	rc = connect(fd, (sockaddr*)&sin, sizeof(sin));
	if (rc < 0) {
		perror("connect");
		::close(fd);
		throw ConnectError("connect", errno);
	}

	fd_ = fd;
}

void NetMessageSocket::close()
{
	if (socketWatcher_.is_active())
		socketWatcher_.stop();

	if (socketTimer_.is_active())
		socketTimer_.stop();

	if (autoClose_ && !(fd_ < 0)) {
		::close(fd_);
		fd_ = -1;
	}
}

void NetMessageSocket::io(ev::io&, int revents)
{
	debug("I/O event received: 0x%04x", revents);
	socketTimer_.stop();

	if (revents & ev::READ)
		if (!handleRead())
			return;

	if (revents & ev::WRITE)
		if (!handleWrite())
			return;

	socketTimer_.start(30, 0);
}

void NetMessageSocket::timeout(ev::timer&, int revents)
{
	debug("I/O operation timed out");
}

void NetMessageSocket::startWrite()
{
	if (socketWatcher_.is_active()) {
		if (socketWatcher_.events & ev::WRITE)
			return;

		socketWatcher_.stop();
	}

	socketWatcher_.set(fd_, ev::WRITE);
	socketWatcher_.start();

	if (socketTimer_.is_active())
		socketTimer_.stop();

	socketTimer_.start(30, 0);
}

bool NetMessageSocket::handleWrite()
{
	ssize_t rc = ::write(fd_, writeBuffer_.data() + writePos_, writeBuffer_.size() - writePos_);
	if (rc < 0) {
		perror("write");
		return false;
	}

	writePos_ += rc;

	if (writePos_ == writeBuffer_.size()) {
		writeBuffer_.clear();
		writePos_ = 0;
		startRead();
	}

	return true;
}

void NetMessageSocket::startRead()
{
	socketWatcher_.set(fd_, ev::READ);
	socketWatcher_.start();

	if (socketTimer_.is_active())
		socketTimer_.stop();

	socketTimer_.start(30, 0);
}

bool NetMessageSocket::handleRead()
{
	char buf[4096];
	ssize_t rc = ::read(fd_, buf, sizeof(buf));

	if (rc < 0) {
		perror("read");
		return false;
	} else if (rc == 0) {
		debug("remote disconnected");
		close();
		return false;
	} else {
		readBuffer_.push_back(buf, rc);
		reader_.parse();

		if (reader_.state() == NetMessageReader::MESSAGE_END) {
			receiveHook_(this);
		} else {
			debug("partial message received: %s (%d)", reader_.state_str(), reader_.state());
		}
		return true;
	}
}

void NetMessageSocket::writeArrayHeader(size_t numValues)
{
	NetMessageWriter::writeArrayHeader(writeBuffer_, numValues);
	startWrite();
}

void NetMessageSocket::writeStatus(const std::string& value)
{
	NetMessageWriter::writeStatus(writeBuffer_, value.c_str());
	startWrite();
}

void NetMessageSocket::writeError(const std::string& value)
{
	NetMessageWriter::writeError(writeBuffer_, value.c_str());
	startWrite();
}

#if (1 == 0)
bool NetMessageSocket::readMessage()
{
	writeMessage("GET", MemRef(key, keysize));
	flush();

	buf_.clear();
	NetMessageReader parser(&buf_);

	socket_->read(buf_);
	parser.parse();

	if (parser.state() != NetMessageReader::MESSAGE_END) {
		// protocol error
		TRACE("protocol error: %d", parser.state());
		return false;
	}

	NetMessage* message = parser.message();

	switch (message->type()) {
		case NetMessage::Array:
		case NetMessage::Status:
		case NetMessage::Number:
			// unexpected result type, but yeah
		case NetMessage::String:
			val.clear();
			val.push_back(message->toString());
			return true;
		case NetMessage::Nil:
			val.clear();
			return true;
		case NetMessage::Error:
		default:
			TRACE("unknown type");
			return false;
	}
}

ssize_t NetMessageSocket::flush()
{
	ssize_t n = socket_->write(buf_.data() + flushPos_, buf_.size() - flushPos_);

	if (n > 0) {
		flushPos_ += n;
		if (flushPos_ == buf_.size()) {
			flushPos_ = 0;
			buf_.clear();
		}
	}
	return n;
}
#endif
// }}}
