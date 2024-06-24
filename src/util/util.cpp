#include "util/util.hpp"

static constexpr char HEX_DIGITS[] = "0123456789ABCDEF";

std::string toHexString8(uint8_t x) {
    std::string output(2, '0');
    output[1] = HEX_DIGITS[x & 0xF];
    output[0] = HEX_DIGITS[(x >> 4) & 0xF];
    return output;
}
std::string toHexString16(uint16_t x) {
    std::string output(4, '0');
    output[3] = HEX_DIGITS[x & 0xF];
    output[2] = HEX_DIGITS[(x >> 4) & 0xF];
    output[1] = HEX_DIGITS[(x >> 8) & 0xF];
    output[0] = HEX_DIGITS[(x >> 12) & 0xF];
    return output;
}