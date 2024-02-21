#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <vector>

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
#ifdef _WIN32
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
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <poll.h>
	#include <fcntl.h>
	#include <errno.h>
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

namespace cpp_socket
{
	struct Packet {
		std::vector<uint8_t> size_in_bytes;
		std::vector<uint8_t> bytes;
		uint8_t size_byte_count = 4;

		int32_t pending_size() {
			if (size_in_bytes.size() < size_byte_count) {
				// Not enough size bytes to determine the total size yet
				return -1;
			}

			int32_t total_size = 
				size_in_bytes[0] << 24 |
				size_in_bytes[1] << 16 |
				size_in_bytes[2] << 8 |
				size_in_bytes[3];

			return total_size - bytes.size();
		}

		void clear() {
			size_in_bytes.clear();
			bytes.clear();
		}
	};

	enum ip_version_t {
		IPV4,
		IPV6
	};

	Packet string_to_packet(std::string s) {
		Packet p;
		for (char c: s) {
			p.bytes.push_back(c);
		}
		
		return p;
	}

	std::string packet_to_string(Packet p) {
		return std::string(p.bytes.begin(), p.bytes.end());
	}

	class Socket
	{
	public:
		Socket(std::string ip, ip_version_t ip_version, int port, bool blocking, uint32_t max_packet_size=8192)
		{
			this->ip_version = ip_version;
			this->max_packet_size = max_packet_size;
			#ifdef _WIN32
				WSADATA wsaData;
				if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
				{
					throw std::runtime_error("Failed to initialize Winsock.");
				}
			#endif

			if (ip_version == IPV4) {
				if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
				{
					throw std::runtime_error("Failed to create socket.");
				}

				m_sockaddr_in.ipv4.sin_family = AF_INET;
				m_sockaddr_in.ipv4.sin_port = htons(port);
			}
			else if (ip_version == IPV6) {
				if ((m_socket = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
				{
					throw std::runtime_error("Failed to create socket.");
				}

				m_sockaddr_in.ipv6.sin6_family = AF_INET6;
				m_sockaddr_in.ipv6.sin6_port = htons(port);
			}
			else {
				throw std::runtime_error("Unsupported ip version.");
			}

			m_pollfd.fd = m_socket;
			
			#ifdef _WIN32
				m_pollfd.events = POLLIN | POLLOUT;
			#else
				m_pollfd.events = POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL;
			#endif

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

			if (ip.length())
			{
				if (ip_version == IPV4) {
					m_sockaddr_in.ipv4.sin_addr.s_addr = inet_addr(ip.c_str());
				}
				else if (ip_version == IPV6) {
					if (inet_pton(AF_INET6, ip.c_str(), &(m_sockaddr_in.ipv6.sin6_addr)) != 1) {
						throw std::runtime_error("Error converting IPv6 address.");
					}
				}
				else {
					throw std::runtime_error("Unsupported ip version.");
				}

				if (connect(m_socket, reinterpret_cast<struct sockaddr *>(&m_sockaddr_in), sizeof(m_sockaddr_in)) == -1)
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
			else
			{
				if (ip_version == IPV4) {
					m_sockaddr_in.ipv4.sin_addr.s_addr = INADDR_ANY;
				}
				else if (ip_version == IPV6) {
					m_sockaddr_in.ipv6.sin6_addr = in6addr_any;
				}
				else {
					throw std::runtime_error("Unsupported ip version.");
				}
				if (bind(m_socket, reinterpret_cast<struct sockaddr *>(&m_sockaddr_in), sizeof(m_sockaddr_in)) == -1)
				{
					throw std::runtime_error("Failed to bind.");
				}

				if (listen(m_socket, 1) == -1)
				{
					throw std::runtime_error("Failed to listen.");
				}
			}
		}

		Socket(SOCKET_TYPE m_socket, sockaddr socketAddress, ip_version_t ip_version, bool blocking, uint32_t max_packet_size=8192)
		{
			this->max_packet_size = max_packet_size;
			this->m_socket = m_socket;
			this->ip_version = ip_version;
			if (ip_version == IPV4) {
				this->m_sockaddr_in.ipv4 = *(reinterpret_cast<sockaddr_in*>(&socketAddress));
			}
			else if (ip_version == IPV6) {
				this->m_sockaddr_in.ipv6 = *(reinterpret_cast<sockaddr_in6*>(&socketAddress));
			}
			else {
				throw std::runtime_error("Unsupported ip version.");
			}
			this->blocking = blocking;
		}

		Socket* accept_connection()
		{
			SOCKET_TYPE clientSocket;
			sockaddr clientAddress;
			socklen_t clientAddressLength = sizeof(clientAddress);

			clientSocket = accept(m_socket, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddressLength);

			if (clientSocket == INVALID_SOCKET) {
				return nullptr;
			}

			if (ip_version == IPV4) {
				return new Socket(clientSocket, clientAddress, ip_version, blocking, max_packet_size);
			}
			else if (ip_version == IPV6) {
				return new Socket(clientSocket, clientAddress, ip_version, blocking, max_packet_size);
			}
			else {
				throw std::runtime_error("Unsupported ip version.");
			}
		}

		/*
		ONLY WORKS WITH accept_connection() and send_packet()
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

		int get_syscall_error() {
			#if _WIN32
				return WSAGetLastError();
			#else
				return errno;
			#endif
		}


		/*
		- Returns -2 if there is an error with package size
		- Returns -1 if there is syscall error
		- Returns 0 if connection is closed
		- Returns 1 if packet is available for dumping
		*/
		int receive_packet() {
			int32_t pending_size = currReceivedPacket.pending_size();
			
			if (pending_size == -1) {
				int r = receive_packet_size();
				if (r < 0) {
					return -1;
				}
				else if (r == 0) {
					return 0; // assuming connection was reset by the other side
				}
				else {
					return receive_packet();
				}
			}
			else if (pending_size == 0 && currReceivedPacket.bytes.size() > 0) {
				received_packets.emplace_back(std::move(currReceivedPacket));
				currReceivedPacket.clear();
				return 1;
			}
			else if (pending_size == 0) {
				return 0; // assuming connection was reset by the other side
			}
			else if (currReceivedPacket.bytes.size() > max_packet_size) {
				return -2;
			}
			else {
				int r = receive_packet_data(pending_size);
				if (r < 0) {
					return -1;
				}
				else if (r == 0) {
					return 0; // assuming connection was reset by the other side
				}
				else {
					return receive_packet();
				}
			}
		}

		/*
		- Returns -2 if there is an error with package size
		- Returns -1 if there is syscall error
		- Returns 0 if connection is closed
		- Returns 1 if packet is completely sent
		*/
		int send_packet(Packet&& packet) {
			if (packet.bytes.size() > max_packet_size || packet.bytes.size() == 0) {
				return -2;
			}
			currSentPacket = std::move(packet);
			int32_t data_len = currSentPacket.bytes.size();
			std::vector<uint8_t> len_in_bytes = {
				static_cast<uint8_t>(data_len >> 24),
				static_cast<uint8_t>(data_len >> 16),
				static_cast<uint8_t>(data_len >> 8),
				static_cast<uint8_t>(data_len),
			};

			currSentPacket.bytes.insert(currSentPacket.bytes.begin(), len_in_bytes.begin(), len_in_bytes.end());

			int len = currSentPacket.bytes.size();
			int r = send(m_socket, reinterpret_cast<char*>(currSentPacket.bytes.data()), len, 0);
			if (r < 0) {
				return -1;
			}
			else if (r == 0) {
				return 0; // assuming connection was reset by the other side
			}
			else if (r < len) {
				currSentPacket.bytes.erase(currSentPacket.bytes.begin(), currSentPacket.bytes.begin() + r);
				return send_packet(std::move(currSentPacket)); // ugly
			}
			else {
				currSentPacket.clear();
				return 1;
			}
		}

		std::vector<Packet> dump_received_packets() {
			std::vector<Packet> received_packets = std::move(this->received_packets);
			this->received_packets.clear();
			return received_packets;
		}

		size_t pending_packet_count() {
			return this->received_packets.size();
		}

		/**
		 * @brief Max packet size, including the size_in_byte count
		 * 
		 * @return uint32_t 
		 */
		uint32_t get_max_packet_size() {
			return max_packet_size;
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

		std::string get_ip() {
			if (ip_version == IPV6) {
				char ipAddr[INET6_ADDRSTRLEN]; // INET6_ADDRSTRLEN is defined in <netinet/in.h> and <winsock2.h>
				if (inet_ntop(AF_INET6, &(m_sockaddr_in.ipv6.sin6_addr), ipAddr, INET6_ADDRSTRLEN) != nullptr) {
					return std::string(ipAddr);
				} else {
					throw std::runtime_error("Error getting ip.");
				}
			}
			else if (ip_version == IPV4) {
				char ipAddr[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN is defined in <netinet/in.h> and <winsock2.h>
				if (inet_ntop(AF_INET, &(m_sockaddr_in.ipv4.sin_addr), ipAddr, INET_ADDRSTRLEN) != nullptr) {
					return std::string(ipAddr);
				} else {
					throw std::runtime_error("Error getting ip.");
				}
			}
			else {
				throw std::runtime_error("Unsupported ip version.");
			}
			
		}

		~Socket()
		{
			end();
		}

	private:
		SOCKET_TYPE m_socket;
		ip_version_t ip_version;
		union {
			sockaddr_in ipv4;
			sockaddr_in6 ipv6;
		} m_sockaddr_in;
		uint32_t max_packet_size;
		bool blocking;
		POLLFD_TYPE m_pollfd;
		Packet currReceivedPacket;
		Packet currSentPacket;
		std::vector<Packet> received_packets;

		/*
		- < 0 for error
		- 0 for graceful disconnect ?
		- 1 for partial size
		- 2 for packet_size_done
		*/
		int receive_packet_size() {
			int len = currReceivedPacket.size_byte_count - currReceivedPacket.size_in_bytes.size();
			char *buf = new char[len];
			int r = recv(m_socket, buf, len, 0);

			if (r <= 0) {
				delete[] buf;
				return r;
			}
			else if (r < len) {
				for (int i=0; i<len; i++) {
					currReceivedPacket.size_in_bytes.push_back(buf[i]);
				}
				delete[] buf;
				return 1;
			}
			else {
				for (int i=0; i<len; i++) {
					currReceivedPacket.size_in_bytes.push_back(buf[i]);
				}
				delete[] buf;
				return 2;
			} 
		}

		/*
		- < 0 for error
		- 0 for graceful disconnect ?
		- 1 for partial size
		- 2 for packet_size_done
		*/
		int receive_packet_data(int len) {
			char *buf = new char[len];
			int r = recv(m_socket, buf, len, 0);
			if (r <= 0) {
				delete[] buf;
				return r;
			}
			else if (r < len) {
				for (int i=0; i<len; i++) {
					currReceivedPacket.bytes.push_back(buf[i]);
				}
				delete[] buf;
				return 1;
			}
			else {
				for (int i=0; i<len; i++) {
					currReceivedPacket.bytes.push_back(buf[i]);
				}
				delete[] buf;
				return 2;
			}
		}
	};

} // namespace cpp_socket

#endif // SOCKET_H