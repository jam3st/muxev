#pragma once

#include <mutex>
#include "pollable.h"

class Router;

class EvDev : public Pollable {
    friend class Router;
public:
    static constexpr char const* const devUinput = "/dev/uinput";
    static constexpr size_t MaxEventsPerRead = 42u;
public:
    EvDev() = delete;
    EvDev(Router& rt);
    virtual ~EvDev();
protected:
    void readAndRoute();
private:
    void write(struct input_event const ev[], size_t const count) const;
private:
    void read(Switch& sw) override final;
    void write(Switch& sw) override final;
    void error(Switch& sw) override;
protected:
    Router& rt;
    enum KeyStateValues : signed int {
        Up = 0,
        Down = 1,
        Repeat = 2
    };
    enum Leds : unsigned char {
        CapsLock = 0x1u,
        ScrollLock = 0x2u,
        NumLock = 0x4u
    };
};

