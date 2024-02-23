#include <Socket.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

using cpp_socket::Socket;
using cpp_socket::Packet;
using cpp_socket::ip_version_t::IPV4;

constexpr int PORT = 8080;
std::mutex m;
std::condition_variable cv;

void print_packet(Packet& packet) {
	for (const auto& byte: packet.bytes) {
		std::cout << byte;
	}
	std::cout << std::endl;
}

int main() {
	try {
		Socket* serverSocket = new Socket("", IPV4, PORT, false);
		std::queue<Socket*> clients;

		std::thread listener([&]() {
			std::cout << "Listening on port " << PORT << std::endl;
			while (true) {
				if (serverSocket->poll_socket() & POLLIN) {
					Socket* clientSocket = serverSocket->accept_connection();

					if (clientSocket != nullptr) {
						std::unique_lock lock(m);
						std::cout << "Client accepted" << std::endl;
						clients.push(clientSocket);
						cv.notify_one();
					}
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		});

		while (true) {
			Socket *clientSocket = nullptr;
			{
				std::unique_lock lock(m);
				cv.wait(lock, [&]{return !clients.empty();});
				clientSocket = clients.front();
				clients.pop();
			}

			int r = clientSocket->receive_packet();

			if (r == -2) {
				std::unique_lock lock(m);
				std::cout << "Malformed package, discarding..." << std::endl;
				clients.push(clientSocket);
			}
			else if (r == -1) {
				int err = clientSocket->get_syscall_error();
				if (err == WOULDBLOCK_ERROR) {
					std::unique_lock lock(m);
					clients.push(clientSocket);
				}
				else {
					std::unique_lock lock(m);
					std::cout << "Error! Code: " << err << std::endl;
					delete clientSocket;
				}
			}	
			else if (r == 0) {
				delete clientSocket;
			}
			else if (r == 1) {
				std::unique_lock lock(m);
				std::vector<Packet> packets = clientSocket->dump_received_packets();

				if (!packets.empty()) {
					for (auto& packet: packets) {
						std::cout << "\t";
						print_packet(packet);
					}
				}
				clients.push(clientSocket);
			}
		}

	} catch (std::runtime_error& e) {
		std::cout << e.what() << std::endl;
	}
	
	Socket::cleanup();

	return 0;
}