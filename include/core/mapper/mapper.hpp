#ifndef MAPPER_HPP
#define MAPPER_HPP

#include "util/serializer.hpp"
#include "util/util.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class Mapper {
public:
    static constexpr uint16_t PRG_ROM_CHUNK_SIZE = 16 * KB;
    static constexpr uint16_t CHR_ROM_CHUNK_SIZE = 8 * KB;

    enum class MirrorMode {
        HORIZONTAL,
        VERTICAL,
        ONE_SCREEN_LOWER_BANK,
        ONE_SCREEN_UPPER_BANK,
        FOUR_SCREEN
    };

    struct Config {
        uint16_t id;
        uint8_t prgChunks;
        uint8_t chrChunks;
        MirrorMode initialMirrorMode;
        bool hasBatteryBackedPrgRam;
        bool alternativeNametableLayout;
    };

    const Config config;

    Mapper(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
    virtual ~Mapper() = default;

    virtual void reset() = 0;

    static std::unique_ptr<Mapper> createMapper(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    // "View" is different from "read" because view functions do not change the state of the mapper.
    // This gives us a way to see the internals of the cartridge without modifying the state of the mapper.
    // This can be useful for debugging.
    // By default, the "read" methods function the same as the "view" methods, but this behavior can be overridden by mappers that change state after reads.
    virtual uint8_t mapPRGView(uint16_t cpuAddress) const = 0;
    virtual uint8_t mapPRGRead(uint16_t cpuAddress);
    virtual void mapPRGWrite(uint16_t cpuAddress, uint8_t value) = 0;

    virtual uint8_t mapCHRView(uint16_t ppuAddress) const = 0;
    virtual uint8_t mapCHRRead(uint16_t ppuAddress);
    virtual void mapCHRWrite(uint16_t ppuAddress, uint8_t value) = 0;

    // By default this returns the mirror mode that was set by the cartridge, but some mappers change the mirroring on their own.
    virtual MirrorMode getMirrorMode() const;

    // Serialization
    virtual void serialize(Serializer& s) const = 0;
    virtual void deserialize(Deserializer& d) = 0;

protected:
    static constexpr MemoryRange PRG_RAM_RANGE{ 0x6000, 0x7FFF };
    std::vector<uint8_t> prgRam;
    bool canAccessPrgRam(uint16_t address) const;
    uint8_t getPrgRam(uint16_t address) const;
    void setPrgRam(uint16_t address, uint8_t value);

    std::vector<uint8_t> prg;
    std::vector<uint8_t> chr;

    static constexpr MemoryRange PRG_RANGE{ 0x8000, 0xFFFF };
    static constexpr MemoryRange CHR_RANGE{ 0x0000, 0x1FFF };
};

#endif // MAPPER_HPP