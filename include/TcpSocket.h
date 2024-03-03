#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "SocketWrapper.h"

namespace cpp_socket::tcp_socket {
	class TcpSocket: public SocketWrapper {
	public:
		TcpSocket(address_family_t ip_protocol, std::string ip, int port, bool blocking)
			:SocketWrapper(ip_protocol, SOCK_STREAM, 0, createAddress(ip_protocol, ip, port), blocking) {
			
		}

		TcpSocket(SOCKET_TYPE m_socket, Address&& address, bool blocking)
			:SocketWrapper(m_socket, std::move(address), blocking) {

		}

		TcpSocket* accept_connection() {
			SOCKET_TYPE clientSocket;
			sockaddr client_sockaddr;
			socklen_t client_sockaddr_size = sizeof(client_sockaddr);

			clientSocket = accept(m_socket, reinterpret_cast<struct sockaddr *>(&client_sockaddr), &client_sockaddr_size);

			if (clientSocket == INVALID_SOCKET) {
				throw std::runtime_error("Error accepting client.");
			}

			Address clientAddress(client_sockaddr, address.get_address_family());

			return new TcpSocket(clientSocket, std::move(clientAddress), blocking);
		}

		/*
		- Returns false if there is already pending data, or size is to big. 
		- To clear pending data, call clear_send().
		- Max size limit is 2^32-1 bytes.
		*/
		bool set_send_data(std::vector<unsigned char> bytes) {
			int size = data_send.size();
			if (size == 0) {
				data_send = bytes;
				std::vector<unsigned char> size_bytes = {
					static_cast<unsigned char>(bytes.size() >> 24),
					static_cast<unsigned char>((bytes.size() >> 16) & 0x000000FF),
					static_cast<unsigned char>((bytes.size() >> 8) & 0x000000FF),
					static_cast<unsigned char>(bytes.size() & 0x000000FF)
				};
				data_send.insert(data_send.begin(), size_bytes.begin(), size_bytes.end());
				return true;
			}

			return false;
		}

		/*
		- Use only if there has been a disconnect.
		*/
		void clear_send() {
			data_send.clear();
		}

		/*
		- Returns -2 if there is an error with data size
		- Returns -1 if there is syscall error
		- Returns 0 if connection is closed
		- Returns 1 if sending is complete
		*/
		int send_data() {
			int size = data_send.size();

			if (size <= 0) {
				return -2;
			}

			int r;
			while ((r = send_wrapper(reinterpret_cast<char*>(data_send.data()), data_send.size(), 0)) < size) {
				if (r == 0) {
					return 0;
				}
				else if (r < 0) {
					return -1;
				}
				data_send.erase(data_send.begin(), data_send.begin() + r);
			}
			return 1;			
		}

		std::vector<unsigned char> dump_received_data() {
			std::vector<unsigned char> data = std::move(data_receive);
			data_receive.clear();
			// we do not clear the index or the size, since they are used in other checks
			return data;
		}

		/*
		- Returns -2 if there is an error with data size
		- Returns -1 if there is syscall error
		- Returns 0 if connection is closed
		- Returns 1 if data is available for dumping
		*/
		int receive_data() {
			// ensure the data is dumped if complete
			if (!data_receive.empty() && data_index_receive == data_receive.size()) {
				return 1; 
			}

			int r;

			// get next data size, if not already received
			while (size_bytes_index_receive < 4) {
				unsigned char remaining = 4 - size_bytes_index_receive;
				r = receive_wrapper(reinterpret_cast<char*>(&size_bytes_receive)+size_bytes_index_receive, remaining, 0);
				if (r == 0) {
					return 0;
				}
				else if (r < 0) {
					return -1;
				}
				size_bytes_index_receive += r;
			}

			// parse data size, if not already received
			if (data_size_receive < 0) {
				data_size_receive = 0;
				data_size_receive = size_bytes_receive[0] << 24 | size_bytes_receive[1] << 16 | size_bytes_receive[2] << 8 | size_bytes_receive[3];
				if (data_size_receive > 0) {
					// reset data_receive_index and allocate memory for new data
					// data_receive.clear(); // data_receive should be cleared already after dumping
					data_receive.resize(data_size_receive);
					data_index_receive = 0;
				}
				else {
					data_size_receive = -1;
					return -2; // error parsing data size
				}
			}

			// keep receiving if index hasn't reached the total data size
			while (data_index_receive < data_size_receive) {
				r = receive_wrapper(reinterpret_cast<char*>(data_receive.data())+data_index_receive, data_size_receive-data_index_receive, 0);
				
				if (r == 0) {
					return 0;
				}
				else if (r < 0) {
					return -1;
				}

				data_index_receive += r;
			}

			// reset total data size and size_bytes
			data_size_receive = -1;
			size_bytes_index_receive = 0;
			// memset(size_bytes_receive, 4, 0x0); 

			return 1;
		}
	private:
		Address createAddress(address_family_t ip_protocol, std::string ip, int port) {
			Address address(ip_protocol);
			address.set_address(ip, port);
			return address;
		}

		std::vector<unsigned char> data_send;

		int data_size_receive = -1;
		unsigned char size_bytes_receive[4] = {0x0, 0x0, 0x0, 0x0};
		std::vector<unsigned char> data_receive;
		
		// starting index to be received, including the start
		// once all bytes are received, index will be len, which will be out of range
		unsigned int data_index_receive = 0;
		unsigned char size_bytes_index_receive = 0; 
	};
} // cpp_socket::tcp_socket

#endif // TCP_SOCKET_H
