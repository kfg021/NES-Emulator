#ifndef MAPPER3_HPP
#define MAPPER3_HPP

#include "core/mapper/mapper.hpp"

#include "util/util.hpp"

class Mapper3 : public Mapper {
public:
    Mapper3(const Config& config, const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);

    void reset() override;
    
    uint8_t mapPRGView(uint16_t cpuAddress) const override;
    void mapPRGWrite(uint16_t cpuAddress, uint8_t value) override;

    uint8_t mapCHRView(uint16_t ppuAddress) const override;
    void mapCHRWrite(uint16_t ppuAddress, uint8_t value) override;

    // Serialization
    void serialize(Serializer& s) const override;
    void deserialize(Deserializer& d) override;

private:
    static constexpr MemoryRange BANK_SELECT_RANGE = PRG_RANGE;
    uint8_t currentBank;
};

#endif // MAPPER3_HPP