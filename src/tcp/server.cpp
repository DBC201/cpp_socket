#include <TcpSocket.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

using cpp_socket::tcp_socket::TcpSocket;
using cpp_socket::SocketWrapper;
using cpp_socket::IPV4;
using cpp_socket::PollWrapper;

constexpr int PORT = 8080;
std::mutex m;
std::condition_variable cv;

int main() {
	try {
		SocketWrapper::startup();
		TcpSocket* serverSocket = new TcpSocket(IPV4, "", PORT, false);
		PollWrapper pollWrapper(serverSocket->get_socket());
		std::queue<TcpSocket*> clients;

		std::thread listener([&]() {
			std::cout << "Listening on port " << PORT << std::endl;
			while (true) {
				if (pollWrapper.poll_socket() & POLLIN) {
					TcpSocket* clientSocket = serverSocket->accept_connection();

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
			TcpSocket *clientSocket = nullptr;
			{
				std::unique_lock lock(m);
				cv.wait(lock, [&]{return !clients.empty();});
				clientSocket = clients.front();
				clients.pop();
			}

			int r = clientSocket->receive_data();

			if (r == -2) {
				std::unique_lock lock(m);
				std::cout << "Malformed package, discarding..." << std::endl;
				clients.push(clientSocket);
			}
			else if (r == -1) {
				int err = cpp_socket::get_syscall_error();
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
				std::vector<unsigned char> data = clientSocket->dump_received_data();
				if (!data.empty()) {
					for (auto& c: data) {
						std::cout << c;
					}
					std::cout << std::endl;
				}
				clients.push(clientSocket);
			}
		}

	} catch (std::runtime_error& e) {
		std::cout << e.what() << std::endl;
		std::cout << "Error code " << cpp_socket::get_syscall_error() << std::endl;
	}
	
	SocketWrapper::cleanup();

	return 0;
}