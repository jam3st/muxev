#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include "evinputdev.h"
#include "exception.h"
#include "router.h"
#include "switch.h"

EvInputDev::EvInputDev(Router& rt, char const* const name) : EvDev(rt), devName(name) {
    fd = -1;
}

void EvInputDev::tryConnect(Switch &sw) {
    auto newFd = ::open(devName, O_RDWR | O_NONBLOCK);
    if(newFd < 0) return;
    throwIf(0 > ::ioctl(newFd, EVIOCGRAB, true), StringException(std::string("Could not grab ")));
    auto me = sw.getFromPtr(this);
    sw.remove(me);
    fd = newFd;
    sw.add(me);
    std::cerr << devName << " connected on: " << fd << std::endl;
}

EvInputDev::~EvInputDev() {
    if(fd >= 0) {
        ::close(fd);
    }
}

void EvInputDev::setLeds(unsigned char ledsState) const {
    struct input_event ev;
    ev.type = EV_LED;
    ev.code = LED_CAPSL;
    if(EvDev::CapsLock == (EvDev::CapsLock & ledsState)) {
        ev.value = 1u;
    } else {
        ev.value = 0u;
    }
    throwIf(0 > ::write(fd, &ev, sizeof(ev)), StringException("Cannot set CAPSLOCK"));

    ev.code = LED_NUML;
    if(EvDev::NumLock == (EvDev::NumLock & ledsState)) {
        ev.value = 1u;
    } else {
        ev.value = 0u;
    }
    throwIf(0 > ::write(fd, &ev, sizeof(ev)), StringException("Cannot set MUMLOCK"));

    ev.code = LED_SCROLLL;
    if(EvDev::ScrollLock == (EvDev::ScrollLock & ledsState)) {
        ev.value = 1u;
    } else {
        ev.value = 0u;
    }
    throwIf(0 > ::write(fd, &ev, sizeof(ev)), StringException("Cannot set SCROLLLOCK"));
}

void EvInputDev::error(Switch &sw) {
    std::cerr << "Attempting reconnect" << std::endl;
    auto me = sw.getFromPtr(this);
    sw.remove(me);
    ::close(fd);
    fd = -1;
    sw.add(me);
    sw.enableTimer();
}
