#include <unix/UnixWrapper.h>
#include <base/PollWrapper.h>

using cpp_socket::unix_wrapper::UnixWrapper;
using cpp_socket::base::PollWrapper;

int main() {
    UnixWrapper unixWrapper("proca", true, true);

    char buf[64];
    unixWrapper.receive_wrapper(buf, 64, 0);
    std::cout << buf << std::endl;
    
    std::string s = "hello world";

    Address* addr = unixWrapper.get_dest_sockaddr("procb", true);

    unixWrapper.sendto_wrapper(s.c_str(), s.size() + 1, 0, addr->get_sockaddr(), addr->size());

    return 0;
}