#include "util.hpp"

std::string toHexString8(uint8_t x){
    const static std::string hexDigits = "0123456789ABCDEF";
    std::string output(2, '0');
    output[1] = hexDigits[x & 0xF];
    output[0] = hexDigits[(x >> 4) & 0xF];
    return output;
}
std::string toHexString16(uint16_t x){
    const static std::string hexDigits = "0123456789ABCDEF";
    std::string output(4, '0');
    output[3] = hexDigits[x & 0xF];
    output[2] = hexDigits[(x >> 4) & 0xF];
    output[1] = hexDigits[(x >> 8) & 0xF];
    output[0] = hexDigits[(x >> 12) & 0xF];
    return output;
}