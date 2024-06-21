#ifndef MAPPER0_HPP
#define MAPPER0_HPP

#include "mapper.hpp"

class Mapper0 : public Mapper{
public:
    Mapper0(uint8_t prgRomChunks, uint8_t chrRomChunks);

    std::optional<uint32_t> mapToPRGView(uint16_t cpuAddress) const override;
    std::optional<uint32_t> mapToPRGRead(uint16_t cpuAddress) override;
    std::optional<uint32_t> mapToPRGWrite(uint16_t cpuAddress) override;
    
    std::optional<uint32_t> mapToCHRView(uint16_t ppuAddress) const override;
    std::optional<uint32_t> mapToCHRRead(uint16_t ppuAddress) override;
    std::optional<uint32_t> mapToCHRWrite(uint16_t ppuAddress) override;
};

#endif // MAPPER0_HPP