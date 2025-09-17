#include <unix/UnixWrapper.h>
#include <base/PollWrapper.h>

using cpp_socket::unix_wrapper::UnixWrapper;
using cpp_socket::base::PollWrapper;

int main() {
    UnixWrapper unixWrapper("procb", true, false);

    std::string s = "hello world";

    Address* addr = unixWrapper.get_dest_sockaddr("proca", true);

    unixWrapper.sendto_wrapper(s.c_str(), s.size() + 1, 0, addr->get_sockaddr(), addr->size());

    PollWrapper pollWrapper(unixWrapper.get_socket());

    while (1) {
        if (pollWrapper.poll_socket() & POLLIN) {
            char buf[64];
            unixWrapper.receive_wrapper(buf, 64, 0);
            std::cout << buf << std::endl;
            break;
        }
    }


    return 0;
}