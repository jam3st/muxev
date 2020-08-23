#pragma once

#include "evdev.h"

class EvInputDev final : public EvDev {
public:
    EvInputDev() = delete;
    EvInputDev(Router& rt, char const* const name);
    virtual ~EvInputDev();
    void setLeds(unsigned char ledsState) const;
    void tryConnect(Switch &sw) override;
    void error(Switch &sw) override;
private:
    char const* const devName;
};
