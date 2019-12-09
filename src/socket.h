#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include "pollable.h"

class Socket : public Pollable {
public:
    typedef union {
          struct sockaddr_in6 addrIn6;
          struct sockaddr_in addrIn;
          struct sockaddr addr;
    } SocketAddress;

    Socket();
    virtual ~Socket();
private:
    void read(Switch& sw) override final;
    void write(Switch& sw) override final;
    void error(Switch& sw) override final;
};
