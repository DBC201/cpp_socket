#include <iostream>
#include <string>
#include <cstring>
#include <Socket.h>

using cpp_socket::Socket;

const std::string SERVER_IP = "127.0.0.1";
const int PORT = 8080;

int main() {
	try {
		Socket serverSocket(SERVER_IP, PORT);
		std::cout << "Connected to server." << std::endl;

		std::string message;
		while (true) {
			std::cout << "Enter a message (or 'quit' to exit): ";
			std::getline(std::cin, message);

			serverSocket.send_message(message);

			if (message == "quit") {
				break;
			}

			std::string receivedMessage = serverSocket.receive_message();
			if (!receivedMessage.empty()) {
				std::cout << "Received echo: " << receivedMessage << std::endl;
			}
		}
	} catch (std::runtime_error& e) {
		std::cout << e.what() << std::endl;
	}
	
	Socket::cleanup();

	return 0;
}
