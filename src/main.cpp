#include <iostream>
#include "switch.h"
#include "router.h"
#include "exception.h"


int main(int const argc, char const* const* argv) {
    try {
        if(3 != argc) {
            std::cerr << "Usage: " << argv[0] << " keyboardDevName mouseDevName" << std::endl <<
                         "   eg: " << argv[0] << " /dev/input/event2 /dev/input/event5" << std::endl;
            throw StringException("Arguments incorrect");
        }
        Switch muxSwitch;
        Router muxer(muxSwitch);
        muxer.init(argv[1], argv[2]);
        muxSwitch.main();
    } catch(StringException const& ex) {
        std::cerr << "Failed: " << ex.why() << std::endl;
        return 1;
    }
    return 0;
}
