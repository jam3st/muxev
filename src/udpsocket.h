#pragma once

#include "socket.h"

class UdpSocket final : public Socket {
public:
    UdpSocket() = delete;
    UdpSocket(void *);
    virtual ~UdpSocket();
};

