#ifndef NETLINK_WRAPPER_H
#define NETLINK_WRAPPER_H

#include <base/SocketWrapper.h>

using cpp_socket::base::SocketWrapper;
using cpp_socket::base::NETLINK;
using cpp_socket::base::Address;

namespace cpp_socket::netlink {
    enum listener_t {
        LINK = RTMGRP_LINK
    };

    enum protocol_t {
        ROUTE = NETLINK_ROUTE
    };

    class NetlinkWrapper: public SocketWrapper {
    public:
        NetlinkWrapper(protocol_t protocol, listener_t listener, bool blocking) 
            :SocketWrapper(NETLINK, SOCK_RAW, protocol, createAddress(listener), blocking) {

        }
    private:
		Address createAddress(listener_t filter) {
			Address address(NETLINK);
			address.set_address("", filter);
			return address;
		}
    };
}

#endif