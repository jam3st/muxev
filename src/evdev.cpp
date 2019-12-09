#include <linux/input.h>
#include <unistd.h>
#include "evdev.h"
#include "router.h"
#include "exception.h"

#include <errno.h>
#include <iostream>

EvDev::EvDev(Router& rt) : rt(rt) {
}

EvDev::~EvDev() {
}

void EvDev::readAndRoute() {
    struct input_event ev[MaxEventsPerRead];
    for(auto i = 0; ; ++i) {
        auto const nr = ::read(fd, &ev[0], sizeof(ev));
        if(0 > nr) {
            if(nr == -1 && (errno == EINTR || errno == EAGAIN)) {
                return;
            } else {
                throw StringException("We are doomed");
                return;
            }
        }
        auto const numEvents = nr / sizeof(ev[0]);

        throwIf(0 == numEvents || 0 != (nr % sizeof(ev[0])) , StringException("Read len wrong"));
        rt.handleInputEvent(this, ev, numEvents);
        if(sizeof(ev) / sizeof(ev[0]) > numEvents) {
            return;
        }
    }
}

void EvDev::write(struct input_event const ev[], size_t const count) const {
    auto const writeSize = count * sizeof(ev[0]);
    auto const nw = ::write(fd, &ev[0], writeSize);
    if(writeSize != nw) {
        if(nw == -1 && (errno == EINTR || errno == EWOULDBLOCK)) {
            throw StringException("Blocking writes not supported.");
        } else {
            throw StringException("Device disconnected.");
        }
    }
}

void EvDev::read(Switch &sw) {
    readAndRoute();
}

void EvDev::write(Switch &sw) {
    std::cerr << "Really?" << std::endl;
}

void EvDev::error(Switch &sw) {
    throw StringException("Device disconnected.");
}
