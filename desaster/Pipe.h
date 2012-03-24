#ifndef sw_x0_Pipe_h
#define sw_x0_Pipe_h (1)

#include <unistd.h>

class Pipe
{
private:
	int pipe_[2];

public:
	explicit Pipe(int flags = 0);
	~Pipe();

	int writeFd() const;
	int readFd() const;

	bool isOpen() const;

	void clear();

	// write to pipe
	ssize_t write(const void* buf, size_t size);
	ssize_t write(Pipe* pipe, size_t size);
	ssize_t write(int fd, size_t size);
	ssize_t write(int fd, off_t *fd_off, size_t size);

	// read from pipe
	ssize_t read(void* buf, size_t size);
	ssize_t read(Pipe* socket, size_t size);
	ssize_t read(int fd, size_t size);
	ssize_t read(int fd, off_t *fd_off, size_t size);
};

// {{{ impl
inline Pipe::~Pipe()
{
	if (isOpen()) {
		::close(pipe_[0]);
		::close(pipe_[1]);
	}
}

inline bool Pipe::isOpen() const
{
	return pipe_[0] >= 0;
}

inline int Pipe::writeFd() const
{
	return pipe_[1];
}

inline int Pipe::readFd() const
{
	return pipe_[0];
}

inline ssize_t Pipe::write(int fd, size_t size)
{
	return write(fd, NULL, size);
}

inline ssize_t Pipe::read(int fd, size_t size)
{
	return read(fd, NULL, size);
}
// }}}

#endif
