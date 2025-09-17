#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#include <iostream>
#include <vector>

#if !defined(_WIN32) && !defined(__unix__)
	#warning "Untested platform detected, only unix and windows are tested."
#endif

#if _WIN32
	#include <winsock2.h>
	#include <Ws2tcpip.h>
	#pragma comment(lib, "ws2_32.lib")
	#define SOCKET_TYPE SOCKET
	#define CLOSE_SOCKET closesocket
	#define POLLFD_TYPE WSAPOLLFD
	#define POLL WSAPoll
	typedef int socklen_t;

	#define WOULDBLOCK_ERROR  WSAEWOULDBLOCK
	#define CONNRESET_ERROR   WSAECONNRESET
	#define CONNABORTED_ERROR WSAECONNABORTED
	#define PIPE_ERROR        WSAEPIPE
	#define INTR_ERROR        WSAEINTR
	#define INVAL_ARGUMENT    WSAEINVAL
	#define NOTSOCK_ERROR     WSAENOTSOCK
#else
	#include <netpacket/packet.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <net/if.h>
	#include <linux/if_ether.h>
	#include <unistd.h>
	#include <poll.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <linux/rtnetlink.h>
	#define SOCKET_TYPE int
	#define CLOSE_SOCKET close
	#define POLLFD_TYPE pollfd
	#define POLL poll
	#define SSIZE_T ssize_t
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1

	#define WOULDBLOCK_ERROR  EAGAIN
	#define CONNRESET_ERROR   ECONNRESET
	#define CONNABORTED_ERROR ECONNABORTED
	#define PIPE_ERROR        EPIPE
	#define INTR_ERROR        EINTR
	#define INVAL_ARGUMENT    EINVAL
	#define NOTSOCK_ERROR     ENOTSOCK
#endif

namespace cpp_socket::base
{
	enum address_family_t {
		IPV4 = AF_INET,
		IPV6 = AF_INET6,
		UNSPECIFIED = AF_UNSPEC,
		#ifdef __unix__
			RAW_PACKET = AF_PACKET,
			NETLINK = AF_NETLINK
		#endif
	};

	class Address {
	public:
		Address(address_family_t address_family) {
			this->address_family = address_family;
		}

		Address() {
			this->address_family = UNSPECIFIED;
		}

		Address(sockaddr p_sockaddr, address_family_t address_family) {
			this->address_family = address_family;
			m_connect_status = 1;
			switch (address_family) {
				case IPV4:
					m_sockaddr.ipv4 = *(reinterpret_cast<sockaddr_in*>(&p_sockaddr));
					break;
				case IPV6:
					m_sockaddr.ipv6 = *(reinterpret_cast<sockaddr_in6*>(&p_sockaddr));
					break;
				default:
					throw std::runtime_error("Unsupported address family.");
			}
		}

		/**
		 * @brief Set the address for given address family
		 * For netlink, address is unused since it is set to be assigned by the kernel
		 * 
		 * @param address 
		 * @param filter 
		 */
		void set_address(std::string address, int filter) {
			int r = 0;
			switch (address_family) {
				case IPV4:
					r = set_ipv4_address(m_sockaddr.ipv4, address, filter);
					if (address.empty()) {
						m_connect_status = 0;
					}
					else {
						m_connect_status = 1;
					}
					break;
				case IPV6:
					r = set_ipv6_address(m_sockaddr.ipv6, address, filter);
					if (address.empty()) {
						m_connect_status = 0;
					}
					else {
						m_connect_status = 1;
					}
					break;
				#ifdef __unix__
				case RAW_PACKET:
					r = set_nic(m_sockaddr.raw, address, filter);
					m_connect_status = -1;
					break;
				case NETLINK:
					r = set_link_listener(m_sockaddr.netlink, filter);
					m_connect_status = -1;
					break;
				#endif
				default:
					throw std::runtime_error("Unsupported address family.");
			}
			if (r != 1) {
				throw std::runtime_error("Error converting address");
			}
		}

		#ifdef __unix__
		static int set_nic(sockaddr_ll& nic, std::string interface, int protocol_filter) {
			nic.sll_family = RAW_PACKET;
			nic.sll_protocol = htons(protocol_filter);
			nic.sll_ifindex = if_nametoindex(interface.c_str());
			return 1;
		}
		
		static int set_link_listener(sockaddr_nl& nl, unsigned int nl_groups) {
			nl.nl_family = NETLINK;
			nl.nl_pid = 0;
			nl.nl_groups = nl_groups;
			return 1;
		}
		#endif

		static int set_ipv4_address(sockaddr_in& ipv4, std::string ip, int port) {
			ipv4.sin_family = IPV4;
			if (port >= 0) {
				ipv4.sin_port = htons(port);
			}
			if (ip.empty()) {
				ipv4.sin_addr.s_addr = INADDR_ANY;
				return 1;
			}
			return inet_pton(IPV4, ip.c_str(), &(ipv4.sin_addr));
		}

		static int set_ipv6_address(sockaddr_in6& ipv6, std::string ip, int port) {
			ipv6.sin6_family = IPV6;
			if (port >= 0) {
				ipv6.sin6_port = htons(port);
			}
			if (ip.empty()) {
				ipv6.sin6_addr = in6addr_any;
				return 1;
			}
			return inet_pton(IPV6, ip.c_str(), &(ipv6.sin6_addr));
		}

		sockaddr* get_sockaddr() {
			return reinterpret_cast<sockaddr*>(&m_sockaddr);
		}

		socklen_t size() {
			return sizeof(m_sockaddr);
		}

		/*
		- -2 means address is not set
		- -1 means it can only bind
		-  0 means it can bind and listen, hence accept
		-  1 means it can connect
		*/
		int connect_status() {
			return m_connect_status;
		}

		address_family_t get_address_family() {
			return address_family;
		}

	private:
		address_family_t address_family;
		union {
			sockaddr_in ipv4;
			sockaddr_in6 ipv6;
			#ifdef __unix__
			sockaddr_ll raw;
			sockaddr_nl netlink;
			#endif
		} m_sockaddr;
		int m_connect_status = -2;
	};

	/*
	Common Error Codes for Non-blocking Socket Operations:

	- EAGAIN (Linux) / EWOULDBLOCK (Linux and Windows):
	Indicates that the operation would block because there is no data available to send or receive at the moment.

	- EINTR (Linux) / WSAEINTR (Windows):
	Indicates that the operation was interrupted by a signal.

	- ECONNRESET (Linux) / WSAECONNRESET (Windows):
	Indicates that the connection has been reset by the remote peer, often due to a graceful closure.

	- ECONNABORTED (Linux) / WSAECONNABORTED (Windows):
	Indicates that the connection was aborted by the local system.

	- EPIPE (Linux) / WSAEPIPE (Windows):
	Indicates that the local end of the socket has been closed, typically when trying to send on a closed socket.

	- EINVAL (Linux) / WSAEINVAL (Windows):
	Indicates that an invalid argument was provided to the function.

	- ENOTSOCK (Linux) / WSAENOTSOCK (Windows):
	Indicates that the file descriptor is not a socket.

	- Other platform-specific error codes may apply; refer to platform documentation for details.
	*/
	int get_syscall_error() {
		#if _WIN32
			return WSAGetLastError();
		#else
			return errno;
		#endif
	}

	class SocketWrapper {
	public:
		static void startup() {
			#ifdef _WIN32
				WSADATA wsaData;
				if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
				{
					throw std::runtime_error("Failed to initialize Winsock.");
				}
			#endif
		}
		
		SocketWrapper(address_family_t address_family, int type, int protocol, Address address, bool blocking) {
			if ((m_socket = socket(address_family, type, protocol)) == -1)
			{
				throw std::runtime_error("Failed to create socket.");
			}

			this->blocking = blocking;

			if (!blocking) {
				#ifdef _WIN32
				u_long nonBlockingMode = 1;
				if (ioctlsocket(m_socket, FIONBIO, &nonBlockingMode) == SOCKET_ERROR) {
					throw std::runtime_error("Failed to set non-blocking mode.");
				}
				#else
					int flags = fcntl(m_socket, F_GETFL, 0);
					if (flags == -1) {
						throw std::runtime_error("Failed to get socket flags.");
					}
					if (fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
						throw std::runtime_error("Failed to set non-blocking mode.");
					}
				#endif
			}

			this->address = address;
			int connect_status = address.connect_status();
			
			switch (connect_status) {
				case -2:
					throw std::runtime_error("Address initialization is wrong.");
				case -1:
					m_bind();
					break;
				case 0:
					m_bind();
					m_listen();
					break;
				case 1:
					m_connect();
					break;
				default:
					throw std::runtime_error("Unrecognized connect status.");
			}
		}

		SocketWrapper(SOCKET_TYPE m_socket, Address&& address, bool blocking) {
			this->m_socket = m_socket;
			this->address = std::move(address);
			this->blocking = blocking;
		}
		
		SocketWrapper* accept_connection()
		{
			SOCKET_TYPE clientSocket;
			sockaddr client_sockaddr;
			socklen_t client_sockaddr_size = sizeof(client_sockaddr);

			clientSocket = accept(m_socket, reinterpret_cast<struct sockaddr *>(&client_sockaddr), &client_sockaddr_size);

			if (clientSocket == INVALID_SOCKET) {
				throw std::runtime_error("Error accepting client.");
			}

			Address clientAddress(client_sockaddr, address.get_address_family());

			return new SocketWrapper(clientSocket, std::move(clientAddress), blocking);
		}

		SOCKET_TYPE get_socket() {
			return m_socket;
		}

		int send_wrapper(const char *buf, int len, int flags) {
			return send(m_socket, buf, len, flags);
		}

		int receive_wrapper(char *buf, int len, int flags) {
			return recv(m_socket, buf, len, flags);
		}

		void end()
		{
			CLOSE_SOCKET(m_socket);
		}

		static void cleanup()
		{
			#ifdef _WIN32
				WSACleanup();
			#endif
		}

		~SocketWrapper()
		{
			end();
		}
	protected:
		SOCKET_TYPE m_socket;
		Address address;
		bool blocking;
	private:
		void m_bind() {
			if (bind(m_socket, address.get_sockaddr(), address.size()) == -1)
			{
				throw std::runtime_error("Failed to bind.");
			}
		}

		void m_connect() {
			if (connect(m_socket, address.get_sockaddr(), address.size()) == -1)
			{
				if (!blocking) {
					// goddamn linux throws EINPROGRESS while windows throws wouldblock
					#ifdef _WIN32
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						throw std::runtime_error("Failed to connect to server.");
					}
					#else
					if (errno != EINPROGRESS) {
						throw std::runtime_error("Failed to connect to server.");
					}
					#endif
				}
				else {
					throw std::runtime_error("Failed to connect to server.");
				}
			}
		}

		void m_listen() {
			if (listen(m_socket, 1) == -1)
			{
				throw std::runtime_error("Failed to listen.");
			}
		}
	};
} // namespace cpp_socket::base

#endif // SOCKET_WRAPPER_H
