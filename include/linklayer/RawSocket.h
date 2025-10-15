#ifndef RAW_SOCKET_H
#define RAW_SOCKET_H

#include <base/SocketWrapper.h>
#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <cstring>

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

        /**
         * @brief Enable and disable pause frames
         * 
         * @param flag 0 or 1 (0 for disabling pause frames)
         * @return int 
         */
        int set_pause_frames(int flag)
        {
            struct {
                struct ethtool_pauseparam ep;
                struct ifreq ifr;
            } u = {0};

            u.ep.cmd = ETHTOOL_SPAUSEPARAM;
            u.ep.autoneg  = 0;
            u.ep.rx_pause = flag;
            u.ep.tx_pause = flag;

            strncpy(u.ifr.ifr_name, ifname.data(), IFNAMSIZ - 1);
            u.ifr.ifr_data = (char *)&u.ep;

            int ret = ioctl(m_socket, SIOCETHTOOL, &u.ifr);
            return ret;
        }

        int ethtool_set_value(int cmd, int val) {
            struct {
                struct ethtool_value ev;
                struct ifreq ifr;
            } u;

            memset(&u, 0, sizeof(u));
            u.ev.cmd  = cmd;
            u.ev.data = val;
            strncpy(u.ifr.ifr_name, ifname.data(), IFNAMSIZ-1);
            u.ifr.ifr_name[IFNAMSIZ-1] = '\0';
            u.ifr.ifr_data = (char *)&u.ev;

            if (ioctl(m_socket, SIOCETHTOOL, &u.ifr) < 0) {
                // Not all drivers support every cmd; treat ENOTSUP as non-fatal
                if (errno == EOPNOTSUPP || errno == EINVAL)
                    return 0;
                return -1;
            }
            return 0;
        }

        int ethtool_clear_flags(int clear_mask) {
            struct {
                struct ethtool_value ev;
                struct ifreq ifr;
            } u;

            memset(&u, 0, sizeof(u));
            u.ev.cmd = ETHTOOL_GFLAGS;
            strncpy(u.ifr.ifr_name, ifname.data(), IFNAMSIZ-1);
            u.ifr.ifr_data = (char *)&u.ev;

            if (ioctl(m_socket, SIOCETHTOOL, &u.ifr) < 0)
                return (errno==EOPNOTSUPP||errno==EINVAL) ? 0 : -1;

            if ((u.ev.data & clear_mask) == 0)
                return 0; // already clear

            u.ev.cmd  = ETHTOOL_SFLAGS;
            u.ev.data &= ~clear_mask;

            if (ioctl(m_socket, SIOCETHTOOL, &u.ifr) < 0)
                return (errno==EOPNOTSUPP||errno==EINVAL) ? 0 : -1;

            return 0;
        }

        int get_mtu() {
            struct ifreq ifr{};
            std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);

            int mtu = 0;

            if (ioctl(m_socket, SIOCGIFMTU, &ifr) == 0) {
                mtu = ifr.ifr_mtu;
            }

            return mtu;
        }

        std::string get_mac_str() {
            std::string mac;
            struct ifreq ifr{};
            std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
            if (ioctl(m_socket, SIOCGIFHWADDR, &ifr) == 0) {
                if (ifr.ifr_hwaddr.sa_family == 1) {
                    const unsigned char* hw = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
                    char buf[18];
                    std::snprintf(buf, sizeof(buf),
                                "%02x:%02x:%02x:%02x:%02x:%02x",
                                hw[0], hw[1], hw[2], hw[3], hw[4], hw[5]);
                    mac.assign(buf);
                }
            }
            return mac;
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
