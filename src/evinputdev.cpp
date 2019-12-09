#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>

#include "evinputdev.h"
#include "exception.h"
#include "router.h"
#include <iostream>

EvInputDev::EvInputDev(Router& rt, char const* const name) : EvDev(rt) {
    fd = ::open(name, O_RDWR | O_NONBLOCK);
    if(fd < 0) return;
    throwIf(0 > ::ioctl(fd, EVIOCGRAB, true), StringException(std::string("Could not grab ")));
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


