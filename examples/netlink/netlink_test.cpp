#include <netlink/NetlinkWrapper.h>

using cpp_socket::netlink::NetlinkWrapper;
using cpp_socket::netlink::LINK;
using cpp_socket::netlink::ROUTE;
using cpp_socket::base::get_syscall_error;

#include <iomanip>

void print_mac(unsigned char* mac) {
	for (int i = 0; i < 6; i++) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
		if (i < 5) std::cout << ":";
	}
	std::cout << std::dec << std::endl;
}


int main() {
    NetlinkWrapper netlinkWrapper(ROUTE, LINK, true);


    int fd = netlinkWrapper.get_socket();


    struct sockaddr_nl me = {0};
    socklen_t len = sizeof(me);
    if (getsockname(fd, (struct sockaddr*)&me, &len) == 0) {
        fprintf(stderr, "portid=%u nl_groups(mask)=0x%x\n", me.nl_pid, me.nl_groups);
    } else {
        perror("getsockname");
    }

    // Modern style: NETLINK_ADD_MEMBERSHIP → list RTNLGRP_* IDs
    unsigned int groups[64] = {0};
    socklen_t glen = sizeof(groups);
    if (getsockopt(fd, SOL_NETLINK, NETLINK_LIST_MEMBERSHIPS, groups, &glen) == 0) {
        size_t n = glen / sizeof(groups[0]);
        bool have_link = false;
        fprintf(stderr, "memberships (RTNLGRP_* IDs):");
        for (size_t i = 0; i < n; ++i) {
            fprintf(stderr, " %u", groups[i]);
            if (groups[i] == RTNLGRP_LINK) have_link = true;
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "subscribed_to_RTNLGRP_LINK=%s\n", have_link ? "yes" : "no");
    } else {
        // Older kernels may return ENOPROTOOPT. Fall back to legacy mask check.
        if (me.nl_groups & RTMGRP_LINK) {
            fprintf(stderr, "legacy mask shows RTMGRP_LINK subscribed\n");
        } else {
            fprintf(stderr, "no RTMGRP_LINK bit set in legacy mask\n");
        }
    }


    while (true) {
        std::vector<char> buf(1  << 17); // 128 KiB
        int n = netlinkWrapper.receive_wrapper(buf.data(), buf.size(), 0);

        if (n < 0) {
            int err = get_syscall_error();
            if (err == EINTR) {
                continue; // since blocking call may get interrupted via kernel mid read
            }

            std::cout << "Error occured with code: " << err << std::endl;
            
            break;
        }

        // Walk all netlink messages in this datagram
        for (nlmsghdr* nh = (nlmsghdr*)buf.data(); NLMSG_OK(nh, (unsigned)n); nh = NLMSG_NEXT(nh, n)) {
            if (nh->nlmsg_type == NLMSG_ERROR) {
                std::cerr << "netlink: NLMSG_ERROR\n";
                continue;
            }
            if (nh->nlmsg_type == NLMSG_DONE) {
                continue;
            }
            if (nh->nlmsg_type != RTM_NEWLINK && nh->nlmsg_type != RTM_DELLINK) {
                // Not a link event; ignore (you can subscribe to more groups and handle here)
                std::cout << " adsiopfjs" << std::endl;
                continue;
            }

            // Header -> ifinfomsg
            auto* ifi = (ifinfomsg*)NLMSG_DATA(nh);
            int ifindex = ifi->ifi_index;
            unsigned ifflags = ifi->ifi_flags;

            // Parse attributes
            char ifname[IF_NAMESIZE] = {0};
            unsigned char* mac = nullptr;
            int maclen = 0;

            int attrlen = nh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));
            for (rtattr* rta = (rtattr*)IFLA_RTA(ifi); RTA_OK(rta, attrlen); rta = RTA_NEXT(rta, attrlen)) {
                switch (rta->rta_type) {
                    case IFLA_IFNAME:
                        std::snprintf(ifname, sizeof(ifname), "%s", (char*)RTA_DATA(rta));
                        break;
                    case IFLA_ADDRESS:
                        mac = (unsigned char*)RTA_DATA(rta);
                        maclen = RTA_PAYLOAD(rta);
                        break;
                    default:
                        break;
                }
            }

            // Print one concise line per event
            std::cout << (nh->nlmsg_type == RTM_NEWLINK ? "NEWLINK/CHANGE" : "DELLINK")
                    << " ifindex=" << ifindex;

            if (ifname[0]) std::cout << " ifname=" << ifname;
            if (nh->nlmsg_type != RTM_DELLINK && mac) {
                std::cout << " mac="; print_mac(mac);
            }
            std::cout << " flags=0x" << std::hex << ifflags << std::dec << "\n";
        }
    }

    return 0;
}

