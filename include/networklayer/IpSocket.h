#ifndef IP_SOCKET_H
#define IP_SOCKET_H

#include <base/SocketWrapper.h>

using cpp_socket::base::SocketWrapper;
using cpp_socket::base::Address;
using cpp_socket::base::address_family_t;

namespace cpp_socket::networklayer {
	class IpSocket: public SocketWrapper {
	public:
		IpSocket(address_family_t addressFamily, bool blocking)
			:SocketWrapper(addressFamily, SOCK_RAW, IPPROTO_RAW, createAddress(addressFamily, "", 0), blocking) {

		}
	private:
		Address createAddress(address_family_t ip_protocol, std::string ip, int port) {
			Address address(ip_protocol);
			address.set_address(ip, port);
			return address;
		}
	};
} // namespace cpp_socket::networklayer


#endif // IP_SOCKET_H
