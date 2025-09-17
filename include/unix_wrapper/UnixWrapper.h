#ifndef NETLINK_WRAPPER_H
#define NETLINK_WRAPPER_H

#include <base/SocketWrapper.h>

using cpp_socket::base::SocketWrapper;
using cpp_socket::base::UNIX_FAM;
using cpp_socket::base::Address;

namespace cpp_socket::unix_wrapper {
    class UnixWrapper: public SocketWrapper {
    public:
        UnixWrapper(std::string name, bool abstract, bool blocking) 
            :SocketWrapper(UNIX_FAM, SOCK_DGRAM, 0, createAddress(name, abstract), blocking) {
        }

        Address* get_dest_sockaddr(std::string name, bool abstract) {
            Address* address = new Address(UNIX_FAM);
            address->set_address(name, abstract ? 1 : 0);
            return address;
        }
    private:
		Address createAddress(std::string name, bool abstract) {
			Address address(UNIX_FAM);
			address.set_address(name, abstract ? 1 : 0);
			return address;
		}
    };
}

#endif