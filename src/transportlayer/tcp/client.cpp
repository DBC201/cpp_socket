#include <transportlayer/TcpSocket.h>
#include <string>
#include <thread>
#include <random>

using cpp_socket::transportlayer::TcpSocket;
using cpp_socket::transportlayer::IPV4;
using cpp_socket::base::SocketWrapper;
using cpp_socket::base::PollWrapper;

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
			SocketWrapper::startup();
			TcpSocket serverSocket(IPV4, SERVER_IP, PORT, false);
			PollWrapper pollWrapper(serverSocket.get_socket());

			int uid = get_random_number(0, pow(2, 8));

			while (true) {
				int p = pollWrapper.poll_socket();

				if (p & POLLOUT) {
					std::string message = std::to_string(uid) + ": hello world";
					std::vector<unsigned char> vec(message.begin(), message.end());
					serverSocket.set_send_data(vec);
					int r = serverSocket.send_data();
					if (r == -2) {
						std::cout << "Invalid package size." << std::endl;
					}
					else if (r == -1) {
						int err = cpp_socket::get_syscall_error();
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

		SocketWrapper::cleanup();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));	
	}

	return 0;
}