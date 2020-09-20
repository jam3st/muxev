#pragma once

#include <memory>
#include <array>
#include "evdev.h"

class Router {
public:
    Router() = delete;
    Router(Switch& sw);
    void init(char const* const inKbdDevName, char const* const inMouseDevName);
    void handleInputEvent(EvDev const* src, struct input_event ev[], size_t const count);
private:
    void processKbdInput(struct input_event ev[], size_t const count);
    void processMouseInput(struct input_event ev[], size_t const count);
private:
    static constexpr size_t NumDisplays = 9u;
private:
    Switch& sw;
    std::shared_ptr<EvDev> inMouse;
    std::shared_ptr<EvDev> inBoard;
    std::array<std::shared_ptr<EvDev>, NumDisplays> outMice;
    std::array<std::shared_ptr<EvDev>, NumDisplays> outBoards;
    std::array<unsigned char, NumDisplays> outLeds;
    size_t activeOutIndex = 1u;
    bool leftCtrlDown = false;
    bool leftAltDown = false;
};
