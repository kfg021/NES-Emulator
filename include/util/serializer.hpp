#ifndef SERIALIZER_HPP
#define SERIALIZER_HPP

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

class Serializer {
public:
    virtual ~Serializer() = default;

    virtual void serializeUInt8(uint8_t data) = 0;
    virtual void serializeUInt16(uint16_t data) = 0;
    virtual void serializeUInt32(uint32_t data) = 0;
    virtual void serializeUInt64(uint64_t data) = 0;
    virtual void serializeInt32(int32_t data) = 0;

    void serializeBool(bool data) {
        serializeUInt8(static_cast<uint8_t>(data));
    }

    // Serialize arrays/vectors of arbitrary type if user supplies a serialization function
    template <typename T, size_t size>
    void serializeArray(const std::array<T, size>& data, const std::function<void(const T&)>& serializeT) {
        for (const T& t : data) {
            serializeT(t);
        }
    }

    template <typename T>
    void serializeVector(const std::vector<T>& data, const std::function<void(const T&)>& serializeT) {
        serializeUInt64(static_cast<uint64_t>(data.size()));
        for (const T& t : data) {
            serializeT(t);
        }
    }

    // Defined here for convenience since serializing arrays and vectors of uint8_t is very common
    const std::function<void(const uint8_t&)> uInt8Func = [&](const uint8_t& data) -> void {
        this->serializeUInt8(data);
    };
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

    // Deserialize arrays/vectors of arbitrary type if user supplies a deserialization function
    template <typename T, size_t size>
    void deserializeArray(std::array<T, size>& data, const std::function<void(T&)>& deserializeT) {
        for (T& t : data) {
            deserializeT(t);
        }
    }

    template <typename T>
    void deserializeVector(std::vector<T>& data, const std::function<void(T&)>& deserializeT) {
        uint64_t sizeDeserialized;
        deserializeUInt64(sizeDeserialized);
        data.resize(sizeDeserialized);
        for (T& t : data) {
            deserializeT(t);
        }
    }

    // Defined here for convenience since deserializing arrays and vectors of uint8_t is very common
    const std::function<void(uint8_t&)> uInt8Func = [&](uint8_t& data) -> void {
        this->deserializeUInt8(data);
    };
};

#endif // SERIALIZER_HPP