#ifndef desaster_Logging_h
#define desaster_Logging_h

#include <vector>
#include <string>

class Logging
{
private:
	static std::vector<char*> env_;

	std::string prefix_;
	std::string className_;
	bool enabled_;

	void updateClassName();
	bool checkEnabled();

public:
	Logging();
	explicit Logging(const char *prefix, ...);

	void setLoggingPrefix(const char *prefix, ...);
	void setLogging(bool enable);

	void debug(const char *fmt, ...);
	void info(const char *fmt, ...);
	void notice(const char *fmt, ...);
	void error(const char *fmt, ...);

private:
	static void initialize();
	static void finalize();
};

#endif
