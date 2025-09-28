#include <linklayer/RawSocket.h>
#include <iomanip>

using cpp_socket::linklayer::RawSocket;
using cpp_socket::linklayer::PROMISCIOUS;

struct EthernetFrame {
	unsigned char destination_mac[6];
	unsigned char source_mac[6];
	unsigned char ether_type[2];
	unsigned char payload[1500];
};

void print_mac(unsigned char* mac) {
	for (int i = 0; i < 6; i++) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
		if (i < 5) std::cout << ":";
	}
	std::cout << std::dec << std::endl;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "usage: ./eth interface" << std::endl;
		return -1;
	}

	RawSocket rawSocket(argv[1], PROMISCIOUS, true);
    rawSocket.set_ignore_outgoing(1);
	while (true) {
		EthernetFrame ethernetFrame;
		int r = rawSocket.receive_wrapper(reinterpret_cast<char*>(&ethernetFrame), sizeof(ethernetFrame), 0);
		if (r <= 0) {
			std::cout << "An error occured while reading from socket with code: " << cpp_socket::base::get_syscall_error() << std::endl;
		}
		else {
			std::cout << "Destination mac: ";
			print_mac(ethernetFrame.destination_mac);
			std::cout << "Source mac: ";
			print_mac(ethernetFrame.source_mac);
			std::cout << "Ether type: ";
			std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ethernetFrame.ether_type[0]);
			std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ethernetFrame.ether_type[1]);
			std::cout << std::dec << std::endl;
			std::cout << "--------------------" << std::endl;
		}
	}
	return 0;
}
