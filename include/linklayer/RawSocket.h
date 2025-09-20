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
		RawSocket(std::string ifname, protocol_t filter, bool blocking) 
			:SocketWrapper(RAW_PACKET, SOCK_RAW, filter, createAddress(ifname, filter), blocking), ifname(ifname) {

		}

        std::string get_ifname() {
            return ifname;
        }
	private:
        std::string ifname;
		Address createAddress(std::string ifname, protocol_t filter) {
			Address address(RAW_PACKET);
			address.set_address(ifname, filter);
			return address;
		}
	};
} // namespace cpp_socket::linklayer

#endif // RAW_SOCKET_H
