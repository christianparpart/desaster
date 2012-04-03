#ifndef desaster_NetMessage_h
#define desaster_NetMessage_h

#include <desaster/Buffer.h>

#include <string>
#include <sstream>

class NetMessage // {{{
{
public:
	enum Type {
		Nil = 0,
		Status = '+',
		Error = '-',
		Number = ':',
		String = '$',
		Array = '*',
	};

private:
	NetMessage();
	NetMessage(Type t, char* buf, size_t size);
	NetMessage(Type t, long long value);

public:
	~NetMessage();

	static NetMessage* createNil();
	static NetMessage* createStatus(const BufferRef& value);
	static NetMessage* createError(const BufferRef& value);
	static NetMessage* createNumber(long long value);
	static NetMessage* createString(const BufferRef& value);
	static NetMessage* createArray(size_t size);

	// type checker
	inline Type type() const { return type_; }
	inline bool isNil() const { return type_ == Nil; }
	inline bool isStatus() const { return type_ == Status; }
	inline bool isError() const { return type_ == Error; }
	inline bool isNumber() const { return type_ == Number; }
	inline bool isString() const { return type_ == String; }
	inline bool isArray() const { return type_ == Array; }

	// value setter
	void setNil();
	void setNumber(long long value);
	void setString(const char* value, size_t size);
	void setArray(size_t size);

	// value getter
	long long toNumber() const { return number_; }
	const char* toString() const { return string_; }
	const NetMessage* toArray() const { return array_; }
	const NetMessage& operator[](size_t i) const { return array_[i]; }
	inline size_t size() const { return number_; }

private:
	Type type_;
	long long number_;
	union {
		char* string_;
		NetMessage* array_;
	};
};
// }}}

class NetMessageParser // {{{
{
public:
	enum State {
		MESSAGE_BEGIN,

		MESSAGE_TYPE,			// $ : + - *
		MESSAGE_LINE_BEGIN,		// ...
		MESSAGE_LINE_OR_CR,		// ... \r
		MESSAGE_LINE_LF,		//     \n
		MESSAGE_NUM_ARGS,		// 123
		MESSAGE_NUM_ARGS_OR_CR,	// 123 \r
		MESSAGE_LF,				//     \n

		BULK_BEGIN,				// $
		BULK_SIZE,				// 1234
		BULK_SIZE_OR_CR,		// 1234 \r
		BULK_SIZE_LF,			//      \n
		BULK_BODY_OR_CR,		// ...  \r
		BULK_BODY_LF,			//      \n

		MESSAGE_END,
		SYNTAX_ERROR
	};

public:
	explicit NetMessageParser(const Buffer* buf);
	~NetMessageParser();

	void parse();

	NetMessage* message() const { return currentContext_->message; }

	inline bool isSyntaxError() const { return state() == SYNTAX_ERROR; }

	inline bool isEndOfBuffer() const { return pos_ == buffer_->size(); }
	inline char currentChar() const { return (*buffer_)[pos_]; }
	inline void nextChar();
	inline size_t nextChar(size_t n);
	inline BufferRef currentValue() const;

	State state() const { return currentContext_->state; }
	const char* state_str() const;
	inline void setState(State st) { currentContext_->state = st; }

private:
	struct ParseContext
	{
		ParseContext() :
			parent(nullptr),
			type(NetMessage::Nil),
			state(MESSAGE_BEGIN),
			number(0),
			sign(false),
			message(nullptr)
		{
		}

		ParseContext* parent;
		NetMessage::Type type;
		State state;			//!< current parser-state
		long long number;		//!< a parsed "number" (array length, string/error/status length, integer value)
		bool sign;				//!< number-sign flag (true if sign-symbol was parsed)
		NetMessage* message;	//!< message
	};

	// global parser state
	const Buffer* buffer_;		//!< buffer holding the message
	size_t pos_;				//!< current parse byte-pos

	ParseContext* currentContext_;

	// bulk context
	size_t begin_;				//!< first byte of currently parsed argument
	ssize_t argSize_;			//!< size of the currently parsed argument

	void pushContext();
	ParseContext* currentContext() const { return currentContext_; }
	void popContext();
}; // }}}

// {{{ NetMessageWriter
/*! writes a raw NetMessage.
 *
 * \code
 * Buffer output;
 * NetMessageWriter::write(output, "hello", 42);
 * \endcode
 */
class NetMessageWriter
{
public:
	typedef std::pair<const char*, size_t> MemRef;

	template<typename Output>
	static void writeArrayHeader(Output& output, size_t arraySize);

	template<typename Output, typename... Args>
	static void write(Output& result, const Args&... args);

	template<typename Output, typename Arg1, typename... Args>
	static void writeValue(Output& result, const Arg1& arg1, const Args&... args);

	template<typename Output> static void writeValue(Output& result, int arg);
	template<typename Output> static void writeValue(Output& result, size_t arg);
	template<typename Output> static void writeValue(Output& result, unsigned long long arg);
	template<typename Output> static void writeValue(Output& result, const char* arg);
	template<typename Output> static void writeValue(Output& result, const std::string& arg);
	template<typename Output> static void writeValue(Output& result, const MemRef& arg);

	template<typename Output> static void writeStatus(Output& result, const char* arg);
	template<typename Output> static void writeError(Output& result, const char* arg);
}; // }}}

// {{{ NetMessageWriter impl
template<typename Output>
void NetMessageWriter::writeArrayHeader(Output& output, size_t arraySize)
{
	output << '*' << arraySize << "\r\n";
}

template<typename Output, typename... Args>
void NetMessageWriter::write(Output& output, const Args&... args)
{
	writeArrayHeader(output, sizeof...(args));
	writeValue(output, args...);
}

template<typename Output, typename Arg1, typename... Args>
void NetMessageWriter::writeValue(Output& output, const Arg1& arg1, const Args&... args)
{
	writeValue(output, arg1);
	writeValue(output, args...);
}

template<typename Output>
void NetMessageWriter::writeValue(Output& output, int arg)
{
	char buf[80];
	int n = std::snprintf(buf, sizeof(buf), "%d", arg);
	output << '$' << n << "\r\n" << arg << "\r\n";
}

template<typename Output>
void NetMessageWriter::writeValue(Output& output, size_t arg)
{
	char buf[80];
	int n = snprintf(buf, sizeof(buf), "%zu", arg);
	output << '$' << n << "\r\n" << arg << "\r\n";
}

template<typename Output>
void NetMessageWriter::writeValue(Output& output, unsigned long long arg)
{
	char buf[80];
	int n = snprintf(buf, sizeof(buf), "%llu", arg);
	output << '$' << n << "\r\n" << arg << "\r\n";
}

template<typename Output>
void NetMessageWriter::writeValue(Output& output, const char* arg)
{
	int n = strlen(arg);
	output << '$' << n << "\r\n" << arg << "\r\n";
}

template<typename Output>
void NetMessageWriter::writeValue(Output& output, const std::string& arg)
{
	output << '$' << arg.size() << "\r\n" << arg << "\r\n";
}

template<typename Output>
void NetMessageWriter::writeValue(Output& output, const MemRef& arg)
{
	output.push_back("$");
	output.push_back(arg.second);
	output.push_back("\r\n");
	output.push_back(arg.first, arg.second);
	output.push_back("\r\n");
}

template<typename Output>
void NetMessageWriter::writeStatus(Output& output, const char* arg)
{
	output << NetMessage::Status << arg << "\r\n";
}

template<typename Output>
void NetMessageWriter::writeError(Output& output, const char* arg)
{
	output << NetMessage::Error << arg << "\r\n";
}
// }}}

#endif
