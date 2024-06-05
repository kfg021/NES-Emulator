#ifndef MAPPER_HPP
#define MAPPER_HPP

#include <stdint.h>
#include <memory>
#include <optional>

class Mapper{
public:
    Mapper(uint8_t prgRomChunks, uint8_t chrRomChunks);
    static std::unique_ptr<Mapper> createMapper(uint8_t id, uint8_t prgRomChunks, uint8_t chrRomChunks);

    virtual std::optional<uint32_t> mapToPRGRead(uint16_t cpuAddress) = 0;
    virtual std::optional<uint32_t> mapToPRGWrite(uint16_t cpuAddress) = 0;

    virtual std::optional<uint32_t> mapToCHRRead(uint16_t ppuAddress) = 0;
    virtual std::optional<uint32_t> mapToCHRWrite(uint16_t ppuAddress) = 0;

protected:
    const uint8_t prgRomChunks;
    const uint8_t chrRomChunks;
};

#endif // MAPPER_HPP