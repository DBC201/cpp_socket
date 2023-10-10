#include <iostream>
#include <string>
#include <cstring>
#include <Socket.h>

using cpp_socket::Socket;

const int PORT = 8080;

int main() {
	try {
		Socket* socket = new Socket("", PORT);
		Socket* clientSocket;

		std::cout << "Server started." << std::endl;

		clientSocket = socket->accept_connection();

		std::cout << "Client connected. Waiting for messages..." << std::endl;

		std::string message;
		while (true) {
			message = clientSocket->receive_message();
			if (message.empty()) {
				break;
			}
			std::cout << "Received message: " << message << std::endl;

			clientSocket->send_message(message);
		}

	} catch (std::runtime_error& e) {
		std::cout << e.what() << std::endl;
	}

	std::cout << "Connection closed." << std::endl;
	
	Socket::cleanup();

	return 0;
}
