#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <cstdint>

class Serializer {
    virtual void serializeUInt8(uint8_t data) = 0;
    virtual void serializeUInt16(uint16_t data) = 0;
    virtual void serializeUInt32(uint32_t data) = 0;
    virtual void serializeUInt64(uint64_t data) = 0;
    virtual void serializeInt32(int32_t data) = 0;

    virtual uint8_t deserializeUInt8() = 0;
    virtual uint16_t deserializeUInt16() = 0;
    virtual uint32_t deserializeUInt32() = 0;
    virtual uint64_t deserializeUInt64() = 0;
    virtual int32_t deserializeInt32() = 0;
};

#endif // SERIALIZER_HPP