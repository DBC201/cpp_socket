#ifndef RAW_SOCKET_H
#define RAW_SOCKET_H

#include <base/SocketWrapper.h>

using cpp_socket::base::SocketWrapper;
using cpp_socket::base::RAW_PACKET;
using cpp_socket::base::Address;

#ifdef _WIN32
	#error "Windows not supported"
#endif

namespace cpp_socket::linklayer {
	enum protocol_t {
		PROMISCIOUS = ETH_P_ALL,
		IPV4_FILTER = ETH_P_IP
	};

	class RawSocket: public SocketWrapper {
	public:
		RawSocket(std::string nic, protocol_t filter, bool blocking) 
			:SocketWrapper(RAW_PACKET, SOCK_RAW, filter, createAddress(nic, filter), blocking) {

		}
	private:
		Address createAddress(std::string nic, protocol_t filter) {
			Address address(RAW_PACKET);
			address.set_address(nic, filter);
			return address;
		}
	};
} // namespace cpp_socket::linklayer

#endif // RAW_SOCKET_H
