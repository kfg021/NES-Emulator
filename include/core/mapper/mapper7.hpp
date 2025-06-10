#ifndef MAPPER7_HPP
#define MAPPER7_HPP

#include "core/mapper/mapper.hpp"

class Mapper7 : public Mapper {
public:
    Mapper7(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    void reset() override;

    uint8_t mapPRGView(uint16_t cpuAddress) const override;
    void mapPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    uint8_t mapCHRView(uint16_t ppuAddress) const override;
    void mapCHRWrite(uint16_t ppuAddress, uint8_t value) override;

    MirrorMode getMirrorMode() const override;

    // Serialization
    void serialize(Serializer& s) const override;
    void deserialize(Deserializer& d) override;

private:
    uint8_t bankSelect;
    PrgRam prgRam;
    ChrRam chrRam;
};

#endif // MAPPER7_HPP