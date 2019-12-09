#pragma once

union UnsignedShortSwap {
      unsigned short ds;
      unsigned char db[sizeof(decltype(ds))];
};

bool const IsBigEndian = []() {
    return 0x3412u == UnsignedShortSwap { .db = { 0x12u, 0x34u } }.ds;
};
