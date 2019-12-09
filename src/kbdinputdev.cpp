#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "kbdinputdev.h"
#include "exception.h"

#include <iostream>

KbdInputDev::KbdInputDev(Router& rt, char const* const devName) : EvDev(rt) {
    fd = ::open(devUinput, O_WRONLY | O_NONBLOCK);
    throwIf(fd < 0, StringException("check /dev/uinput"));

    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_SYN), StringException(std::string("EV_KEY")));
    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_KEY), StringException(std::string("EV_KEY")));
    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_REP), StringException(std::string("EV_REP")));
    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_LED), StringException(std::string("EV_LED")));
    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_MSC), StringException(std::string("EV_MSC")));
    throwIf(0 > ::ioctl(fd, UI_SET_LEDBIT, LED_NUML), StringException(std::string("LED_NUML")));
    throwIf(0 > ::ioctl(fd, UI_SET_LEDBIT, LED_CAPSL), StringException(std::string("LED_CAPSL")));
    throwIf(0 > ::ioctl(fd, UI_SET_LEDBIT, LED_SCROLLL), StringException(std::string("LED_SCROLLL")));
    for(auto i = KEY_ESC; i <= KEY_F24; ++i) {
        throwIf(0 > ::ioctl(fd, UI_SET_KEYBIT, i), StringException(std::string("UI_SET_KEYBIT0 " + std::to_string(i))));
    }
    for(auto i = KEY_OK; i <= KEY_NUMERIC_D; ++i) {
        throwIf(0 > ::ioctl(fd, UI_SET_KEYBIT, i), StringException(std::string("UI_SET_KEYBIT1 " + std::to_string(i))));

    }
    const auto devNameLen = std::strlen(devName);
    struct uinput_setup dev = { 0 };
    dev.id.bustype = BUS_VIRTUAL;
    dev.id.vendor = 0;
    dev.id.product = 0;
    dev.id.version = 0;
    throwIf(sizeof(dev.name) <= (devNameLen + 1), StringException("Retared long dev name"));
    throwIf(nullptr == ::strncpy(dev.name, devName, sizeof(dev.name) - 1), StringException("Seriously?"));
    throwIf(0 > ::ioctl(fd, UI_DEV_SETUP, &dev), StringException(std::string("UI_DEV_SETUP")));
    throwIf(0 > ::ioctl(fd, UI_DEV_CREATE), StringException(std::string("UI_DEV_CREATE")));
}

KbdInputDev::~KbdInputDev() {
    if(fd >= 0) {
        ::ioctl(fd, UI_DEV_DESTROY);
        ::close(fd);
    }
}
