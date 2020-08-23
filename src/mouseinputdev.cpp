#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "mouseinputdev.h"
#include "exception.h"

#include <iostream>

MouseInputDev::MouseInputDev(Router& rt, char const* const devName) : EvDev(rt) {
    fd = ::open(devUinput, O_WRONLY | O_NONBLOCK);
    throwIf(fd < 0, StringException("check /dev/uinput for mouse"));

    throwIf(0 > ::fcntl(fd, F_SETFL, O_NONBLOCK), StringException(std::string("I cannot handle blocking IO on ") + devName));
    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_SYN), StringException(std::string("EV_KEY")));
    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_KEY), StringException(std::string("EV_KEY")));
    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_REL), StringException(std::string("EV_REL")));
    throwIf(0 > ::ioctl(fd, UI_SET_EVBIT, EV_MSC), StringException(std::string("EV_MSC")));
    throwIf(0 > ::ioctl(fd, UI_SET_RELBIT, REL_WHEEL), StringException(std::string("REL_WHEEL")));
    throwIf(0 > ::ioctl(fd, UI_SET_RELBIT, REL_X), StringException(std::string("REL_X")));
    throwIf(0 > ::ioctl(fd, UI_SET_RELBIT, REL_Y), StringException(std::string("REL_Y")));
    throwIf(0 > ::ioctl(fd, UI_SET_KEYBIT, BTN_LEFT), StringException(std::string("BTN_LEFT")));
    throwIf(0 > ::ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE), StringException(std::string("BTN_MIDDLE")));
    throwIf(0 > ::ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT), StringException(std::string("BTN_RIGHT")));
    throwIf(0 > ::ioctl(fd, UI_SET_MSCBIT, MSC_SCAN), StringException(std::string("MSC_SCAN")));
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

MouseInputDev::~MouseInputDev() {
    if(fd >= 0) {
        ::ioctl(fd, UI_DEV_DESTROY);
        ::close(fd);
    }
}


