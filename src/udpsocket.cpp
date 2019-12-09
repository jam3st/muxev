#include "udpsocket.h"
#include "exception.h"

UdpSocket::UdpSocket(void *) {
    fd = ::socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    throwIf(0 > fd, StringException("UDPSOCK"));
    long one = 1;
    throwIf(0 > ::setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &one, sizeof(one)), StringException("RCVPKTINFO"));
}

UdpSocket::~UdpSocket() {

}
