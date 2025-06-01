#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

class Serializer {
public:
    virtual ~Serializer() = default;

    virtual void serializeUInt8(uint8_t data) const = 0;
    virtual void serializeUInt16(uint16_t data) const = 0;
    virtual void serializeUInt32(uint32_t data) const = 0;
    virtual void serializeUInt64(uint64_t data) const = 0;
    virtual void serializeInt(int32_t data) const = 0;

    void serializeBool(bool data) const {
        serializeUInt8(static_cast<uint8_t>(data));
    }

    template <typename T, size_t size>
    void serializeArray(const std::array<T, size>& data, std::function<void(const T&)> serializeT) const {
        for (const T& t : data) {
            serializeT(t);
        }
    }

    template <typename T>
    void serializeVector(const std::vector<T>& data, std::function<void(const T&)> serializeT) const {
        serializeUInt64(static_cast<uint64_t>(data.size()));
        for (const T& t : data) {
            serializeT(t);
        }
    }
};

class Deserializer {
public:
    virtual ~Deserializer() = default;

    virtual void deserializeUInt8(uint8_t& data) = 0;
    virtual void deserializeUInt16(uint16_t& data) = 0;
    virtual void deserializeUInt32(uint32_t& data) = 0;
    virtual void deserializeUInt64(uint64_t& data) = 0;
    virtual void deserializeInt32(int32_t& data) = 0;

    void deserializeBool(bool& data) {
        uint8_t temp;
        deserializeUInt8(temp);
        data = static_cast<bool>(temp);
    }

    template <typename T, size_t size>
    void deserializeArray(std::array<T, size>& data, std::function<void(T&)> deserializeT) {
        for (T& t : data) {
            deserializeT(t);
        }
    }

    template <typename T>
    void deserializeVector(std::vector<T>& data, std::function<void(T&)> deserializeT) {
        uint64_t sizeDeserialized;
        deserializeUInt64(sizeDeserialized);
        data.resize(sizeDeserialized);
        for (T& t : data) {
            deserializeT(t);
        }
    }
};

#endif // SERIALIZER_HPP