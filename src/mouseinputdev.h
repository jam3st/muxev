#pragma once

#include "evdev.h"

class MouseInputDev final : public EvDev  {
public:
    MouseInputDev() = delete;
    MouseInputDev(Router& rt, char const* const devName);
    virtual ~MouseInputDev();
};
