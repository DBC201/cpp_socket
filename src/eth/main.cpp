#include <RawSocket.h>

using cpp_socket::raw_socket::RawSocket;
using cpp_socket::raw_socket::PROMISCIOUS;

int main() {
	RawSocket rawSocket("wlp48s0", PROMISCIOUS);
	while (true) {
		char buffer[1024];
		rawSocket.receive_wrapper(buffer, 1024, 0);
		for (int i=0; i<1024; i++) {
			std::cout << buffer[i];
		}
		std::cout << std::endl;
	}
	return 0;
}
