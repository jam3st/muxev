#pragma once

#include "evdev.h"

class EvInputDev final : public EvDev {
public:

public:
    EvInputDev() = delete;
    EvInputDev(Router& rt, char const* const name);
    virtual ~EvInputDev();
    void setLeds(unsigned char ledsState) const;
};
