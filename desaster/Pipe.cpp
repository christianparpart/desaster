#include "Pipe.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/* creates a pipe
 *
 * \param flags an OR'ed value of O_NONBLOCK and O_CLOEXEC
 */
Pipe::Pipe(int flags)
{
	if (::pipe2(pipe_, flags) < 0) {
		pipe_[0] = -errno;
		pipe_[1] = -1;
	}
}

void Pipe::clear()
{
	char buf[4096];
	ssize_t rv;

	do rv = ::read(readFd(), buf, sizeof(buf));
	while (rv > 0);
}

ssize_t Pipe::write(const void* buf, size_t size)
{
	return ::write(writeFd(), buf, size);
}

ssize_t Pipe::write(Pipe* pipe, size_t size)
{
	return splice(pipe->readFd(), NULL, writeFd(), NULL, 4096, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
}

ssize_t Pipe::write(int fd, off_t* fd_off, size_t size)
{
	return splice(fd, fd_off, writeFd(), NULL, size, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
}

ssize_t Pipe::read(void* buf, size_t size)
{
	return ::read(readFd(), buf, size);
}

ssize_t Pipe::read(Pipe* pipe, size_t size)
{
	return pipe->write(this, size);
}

ssize_t Pipe::read(int fd, off_t* fd_off, size_t size)
{
	return splice(readFd(), fd_off, fd, NULL, size, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
}
