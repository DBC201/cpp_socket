# cpp_socket
A cross platform, high level socket library that runs on linux and windows.

Polling works on send() and accept().

## Usage
```
Socket("", IP_VERSION, PORT, BLOCKING);
Socket(IP, IP_VERSION, PORT, BLOCKING);
```

IP can be an empty string, which will bind to INADDR_ANY.

IP_VERSION is an enum in cpp_socket namespace. It can be accessed via ```cpp_socket::ip_version_t::IPV4``` or ```cpp_socket::ip_version_t::IPV6```.

PORT is the port number.

BLOCKING is a boolean, which will set the socket to blocking or non blocking mode.

### Nonblocking Examples
Listening for connections:
```
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
	}
});
```

Sending messages:
```
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
```
Receiving messages:
```
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
```

## TODO
- [x] Fix polling for windows
- [x] Hard to tell when the server disconnects from client when nonblocking, especially on Linux.
- [x] More consistent errors for send and receive, preferrably don't use SSIZE_T.
- [ ] Add blocking examples to readme.
- [ ] Add ```__linux__``` and ```__APPLE__``` macros, and ```#error``` for other platforms.
- [x] Add ipv6 support.
  