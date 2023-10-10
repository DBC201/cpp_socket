#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <string>
#include <cstring>

#ifdef _WIN32
	#include <winsock2.h>
	#pragma comment(lib, "ws2_32.lib")
	#define SOCKET_TYPE SOCKET
	#define CLOSE_SOCKET closesocket
#else
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#define SOCKET_TYPE int
	#define CLOSE_SOCKET close
#endif

#ifndef socklen_t
typedef int socklen_t;
#endif

namespace cpp_socket
{
	class Socket
	{
	public:
		Socket(std::string ip, int port)
		{
			this->ip = ip;
			this->port = port;

			#ifdef _WIN32
				WSADATA wsaData;
				if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
				{
					throw std::runtime_error("Failed to initialize Winsock.");
				}
			#endif

			if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			{
				throw std::runtime_error("Failed to create socket.");
			}

			socketAddress.sin_family = AF_INET;
			socketAddress.sin_port = htons(port);

			if (ip.length())
			{
				socketAddress.sin_addr.s_addr = inet_addr(ip.c_str());
				if (connect(m_socket, reinterpret_cast<struct sockaddr *>(&socketAddress), sizeof(socketAddress)) == -1)
				{
					throw std::runtime_error("Failed to connect to server.");
				}
			}
			else
			{
				socketAddress.sin_addr.s_addr = INADDR_ANY;
				if (bind(m_socket, reinterpret_cast<struct sockaddr *>(&socketAddress), sizeof(socketAddress)) == -1)
				{
					throw std::runtime_error("Failed to bind.");
				}

				if (listen(m_socket, 1) == -1)
				{
					throw std::runtime_error("Failed to listen.");
				}
			}
		}

		Socket(SOCKET_TYPE m_socket, sockaddr_in socketAddress)
		{
			this->m_socket = m_socket;
			this->socketAddress = socketAddress;
		}

		Socket *accept_connection()
		{
			SOCKET_TYPE clientSocket;
			sockaddr_in clientAddress;
			socklen_t clientAddressLength = sizeof(clientAddress);

			if ((clientSocket = accept(m_socket, reinterpret_cast<struct sockaddr *>(&clientAddress),
									   &clientAddressLength)) == -1)
			{
				throw std::runtime_error("Failed to accept connection.");
			}

			return new Socket(clientSocket, clientAddress);
		}

		std::string receive_message()
		{
			int messageSize = 0;
			if (recv(m_socket, reinterpret_cast<char *>(&messageSize), sizeof(messageSize), 0) == -1)
			{
				throw std::runtime_error("Failed to receive message size.");
			}

			char buffer[1024];
			memset(buffer, 0, sizeof(buffer));

			if (recv(m_socket, buffer, messageSize, 0) == -1)
			{
				throw std::runtime_error("Failed to receive message.");
			}

			return std::string(buffer);
		}

		void send_message(std::string message)
		{
			int messageSize = message.size();
			if (send(m_socket, reinterpret_cast<const char *>(&messageSize), sizeof(messageSize), 0) == -1)
			{
				throw std::runtime_error("Failed to send message size.");
			}

			if (send(m_socket, message.c_str(), messageSize, 0) == -1)
			{
				throw std::runtime_error("Failed to send message.");
			}
		}

		void close()
		{
			CLOSE_SOCKET(m_socket);
		}

		static void cleanup()
		{
			#ifdef _WIN32
				WSACleanup();
			#endif
		}

		~Socket()
		{
			close();
		}

	private:
		std::string ip;
		int port;
		SOCKET_TYPE m_socket;
		sockaddr_in socketAddress;
	};

} // namespace cpp_socket

#endif // SOCKET_H
