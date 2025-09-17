# cpp_socket
A cross platform, high level socket library that runs on linux and windows.
See server and client for example implementation. 

## Transport Layer
### TcpSocket
This class can be used to initiate a tcp connection between server and client based on the parameters given during initialization.

See ```examples/transportlayer```.

## Link/Network Layer (Linux Only)
This class (RawSocket) is used to create raw IP or ethernet sockets.

See ```examples/linklayer```.
