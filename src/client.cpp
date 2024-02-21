#include <string>
#include <Socket.h>
#include <thread>
#include <random>

using cpp_socket::Socket;
using cpp_socket::Packet;
using cpp_socket::ip_version_t::IPV4;

const std::string SERVER_IP = "127.0.0.1";
constexpr int PORT = 8080;

int get_random_number(int minval, int maxval) {
	// Seed the random number generator with a random device
	std::random_device rd;
	std::mt19937 gen(rd());

	// Define a distribution (e.g., uniform distribution [1, 100))
	std::uniform_int_distribution<int> distribution(minval, maxval);

	// Generate a random number
	int random_number = distribution(gen);
	return random_number;
}

int main() {	
	while (true) {
		try {
			std::cout << "Attempting to connect..." << std::endl;
			Socket serverSocket(SERVER_IP, IPV4, PORT, false);

			int uid = get_random_number(0, pow(2, 8));

			while (true) {
				int p = serverSocket.poll_socket();

				if (p & POLLIN) {
					int r = serverSocket.receive_packet();
					if (r == 0) {
						std::cout << "Connection closed gracefully." << std::endl;
						break;
					}
					else if (r == -1 && serverSocket.get_syscall_error() == WOULDBLOCK_ERROR) {
						continue;
					}
					else {
						std::cout << "An error occured." << std::endl;
						break;
					}
				}

				if (p & POLLOUT) {
					Packet packet;
					std::string message = std::to_string(uid) + ": hello world";
					packet.bytes.assign(message.begin(), message.end());
					int r = serverSocket.send_packet(std::move(packet));
					if (r == -2) {
						std::cout << "Invalid package size." << std::endl;
					}
					else if (r == -1) {
						int err = serverSocket.get_syscall_error();
						if (err != WOULDBLOCK_ERROR) {
							std::cout << "Error! Code: " << err << std::endl;
							break;
						}
					}
					else if (r == 0) {
						std::cout << "Connection closed gracefully." << std::endl;
						break;
					}
					else if (r == 1) {
						std::cout << "Message sent." << std::endl;
					}
				}
				else {
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));	
			}
		} catch (std::runtime_error& e) {
			std::cout << e.what() << std::endl;
			std::cout << "Unable to connect." << std::endl;
		}

		Socket::cleanup();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));	
	}

	return 0;
}