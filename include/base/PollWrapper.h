#ifndef POLL_WRAPPER_H
#define POLL_WRAPPER_H

#include "SocketWrapper.h"

namespace cpp_socket::base {
	class PollWrapper {
	public:
		PollWrapper(SOCKET_TYPE m_socket) {
			m_pollfd.fd = m_socket;
			#ifdef _WIN32
				m_pollfd.events = POLLIN | POLLOUT;
			#else
				m_pollfd.events = POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL;
			#endif
		}

		/*
		Monitor the file descriptor for multiple events:
		- POLLIN: Data available for reading (normal and out-of-band data).
		- POLLOUT: Ready for writing data.
		- POLLERR: An error condition has occurred on the file descriptor.
		- POLLHUP: The other end of the socket has hung up or closed the connection.
		- POLLNVAL: The file descriptor is not open or is not valid.

		- USAGE: socket->poll_socket() & POLLOUT
		*/
		int poll_socket() {
			int r = POLL(&m_pollfd, 1, 0);

			if (r == SOCKET_ERROR) {
				throw std::runtime_error("Polling failed.");
			}

			return m_pollfd.revents;
		}
	private:
		POLLFD_TYPE m_pollfd;
	};
} // namespace cpp_socket::base


#endif // POLL_WRAPPER_H
