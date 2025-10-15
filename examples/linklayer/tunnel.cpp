#include <linklayer/RawSocket.h>
#include <thread>

using cpp_socket::linklayer::RawSocket;
using cpp_socket::linklayer::PROMISCIOUS;

void tunnel(RawSocket& i1, RawSocket& i2) {
    while (true) {
        char buffer[3000];
        int r = i1.receive_wrapper(buffer, sizeof(buffer), 0);

        if (r < 0) {
            std::string error_message = "Error reading from " + i1.get_ifname();
            perror(error_message.c_str());
            continue;
        }

        r = i2.send_wrapper(buffer, r, 0);

        if (r < 0) {
            std::string error_message = "Error reading from " + i2.get_ifname();
            perror(error_message.c_str());
        }
    }
}

int main(int argc, char** argv) {

    if (argc != 3) {
        std::cerr << "Usage: sudo ./tunnel <interface1> <interface2>" << std::endl;
        return 0;
    }

    std::string ifname1(argv[1]);
    std::string ifname2(argv[2]);

    RawSocket i1(ifname1, PROMISCIOUS, true);
    RawSocket i2(ifname2, PROMISCIOUS, true);

    i1.set_ignore_outgoing(1);
    i2.set_ignore_outgoing(1);

    std::thread i1_thread([&]() {
        tunnel(i1, i2);
    });

    std::thread i2_thread([&]() {
        tunnel(i2, i1);
    });

    i1_thread.join();
    i2_thread.join();

    return 0;
}

