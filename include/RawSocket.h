#ifndef RAW_SOCKET_H
#define RAW_SOCKET_H

#include "SocketWrapper.h"

namespace cpp_socket::raw_socket {
	enum protocol_t {
		PROMISCIOUS = ETH_P_ALL
	};

	class RawSocket: public SocketWrapper {
	public:
		RawSocket(std::string nic, protocol_t filter) 
			:SocketWrapper(RAW_PACKET, SOCK_RAW, filter, createAddress(nic, filter), false) {

		}
	private:
		Address createAddress(std::string nic, protocol_t filter) {
			Address address(RAW_PACKET);
			address.set_address(nic, filter);
			return address;
		}
	};
} // namespace cpp_socket::raw_socket

#endif // RAW_SOCKET_H
