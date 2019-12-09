#pragma once

#include "evdev.h"

class KbdInputDev final : public EvDev {
public:
    KbdInputDev() = delete;
    KbdInputDev(Router& rt,  char const* const devName);
    virtual ~KbdInputDev();
};
