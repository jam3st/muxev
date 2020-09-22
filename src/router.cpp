#include <linux/input.h>

#include "router.h"
#include "switch.h"
#include "evinputdev.h"
#include "kbdinputdev.h"
#include "mouseinputdev.h"
#include "exception.h"

#include <iostream>
#include <string>

Router::Router(Switch& sw) : sw(sw) {

}

void Router::init(char const* const inKbdDevName, char const* const inMouseDevName) {
    constexpr char NoneString[] = "none";

    if(0 != std::string(NoneString).compare(std::string(inKbdDevName))) {
        inBoard = std::make_shared<EvInputDev>(*this, inKbdDevName);
        sw.add(inBoard);
        for(auto i = 0u; i < NumDisplays; ++i) {
            outLeds[i] = 0u;
            outBoards[i] = std::make_shared<KbdInputDev>(*this, ("evmux-kbd-event-" + std::to_string(i)).c_str());
            sw.add(outBoards[i], false);
        }
    }

    if(0 != std::string(NoneString).compare(std::string(inMouseDevName))) {
        inMouse = std::make_shared<EvInputDev>(*this, inMouseDevName);
        fprintf(stderr, "Mosedev %s : %d\n", inMouseDevName, inMouse.get()->fd);
        sw.add(inMouse);
        for (auto i = 0u; i < NumDisplays; ++i) {
            outLeds[i] = 0u;
            outMice[i] = std::make_shared<MouseInputDev>(*this, ("evmux-mouse-event-" + std::to_string(i)).c_str());
            sw.add(outMice[i], false);
        }
    }
}

void Router::handleInputEvent(EvDev const* src, struct input_event const ev[], size_t const count) {
    if(activeOutIndex == 0u) {
        // TODO
    } else {
        if(inBoard.get() == src) {
            processKbdInput(ev, count);
        } else if(inMouse.get() == src) {
            processMouseInput(ev, count);
        } else {
            throw StringException("I am confuzed");
        }
    }
}

void Router::processMouseInput(struct input_event const ev[], size_t const count) {
    auto& ref = outMice[activeOutIndex - 1u];
    ref->write(ev, count);
}

void Router::processKbdInput(struct input_event const ev[], size_t const count) {
    auto& ref = outBoards[activeOutIndex - 1u];
    auto prevActiveOut = activeOutIndex;
    bool ledsChanged = false;
    for(auto i = 0u; i < count; ++i) {
        auto lastKeyUp = KEY_CNT;
        if(ev[i].type == EV_KEY) {
            if(EvDev::KeyStateValues::Up == ev[i].value) {
                lastKeyUp = ev[i].code;
                if(KEY_LEFTCTRL == ev[i].code) {
                    leftCtrlDown = false;
                } else if(KEY_LEFTALT == ev[i].code) {
                    leftAltDown = false;
                }
            } else if(EvDev::KeyStateValues::Down  == ev[i].value) {
                if(KEY_LEFTCTRL == ev[i].code) {
                    leftCtrlDown = true;
                } else if(KEY_LEFTALT == ev[i].code) {
                    leftAltDown = true;
                } else if(KEY_CAPSLOCK == ev[i].code) {
                    ledsChanged = true;
                    outLeds[activeOutIndex - 1u] ^= EvDev::Leds::CapsLock;
                } else if(KEY_NUMLOCK == ev[i].code) {
                    ledsChanged = true;
                    outLeds[activeOutIndex - 1u] ^= EvDev::Leds::NumLock;
                } else if(KEY_SCROLLLOCK == ev[i].code) {
                    ledsChanged = true;
                    outLeds[activeOutIndex - 1u] ^= EvDev::Leds::ScrollLock;
                }
            }
            if(leftCtrlDown && leftAltDown && KEY_1 <= lastKeyUp && KEY_9 >= lastKeyUp) {
                 activeOutIndex = lastKeyUp - KEY_1 + 1u;
            }
        }
    }
    if(ledsChanged || prevActiveOut != activeOutIndex) {
        auto const* keyBoard = dynamic_cast<EvInputDev*>(inBoard.get());
        auto const ledState = outLeds[activeOutIndex - 1u];
        keyBoard->setLeds(ledState);
    }
    ref->write(ev, count);
}
