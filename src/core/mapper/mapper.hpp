#ifndef MAPPER_HPP
#define MAPPER_HPP

#include <stdint.h>
#include <memory>
#include <optional>

class Mapper {
public:
    Mapper(uint8_t prgRomChunks, uint8_t chrRomChunks);
    virtual ~Mapper() = default;

    static std::unique_ptr<Mapper> createMapper(uint8_t id, uint8_t prgRomChunks, uint8_t chrRomChunks);

    // "View" is different from "read" because view functions do not change the state of the mapper.
    // This gives us a way to see the internals of the cartridge without modifying the state of the mapper.
    // This can be useful for debugging.
    virtual std::optional<uint32_t> mapToPRGView(uint16_t cpuAddress) const = 0;
    virtual std::optional<uint32_t> mapToPRGRead(uint16_t cpuAddress) = 0;
    virtual std::optional<uint32_t> mapToPRGWrite(uint16_t cpuAddress, uint8_t value) = 0;

    virtual std::optional<uint32_t> mapToCHRView(uint16_t ppuAddress) const = 0;
    virtual std::optional<uint32_t> mapToCHRRead(uint16_t ppuAddress) = 0;
    virtual std::optional<uint32_t> mapToCHRWrite(uint16_t ppuAddress, uint8_t value) = 0;

protected:
    const uint8_t prgRomChunks;
    const uint8_t chrRomChunks;
};

#endif // MAPPER_HPP