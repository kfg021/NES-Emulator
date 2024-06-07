#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include "mapper/mapper.hpp"

#include <stdint.h>
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <optional>

class Cartridge{
public:
    Cartridge();
    Cartridge(const std::string& filePath);
    bool isValid() const;

    std::optional<uint8_t> readFromPRG(uint16_t preMappedAddr);
    void writeToPRG(uint16_t preMappedAddr, uint8_t data);
    std::optional<uint8_t> readFromCHR(uint16_t preMappedAddr);
    void writeToCHR(uint16_t preMappedAddr, uint8_t data);

private:
    bool loadINESFile(const std::string& filePath);

    static constexpr uint16_t PRG_ROM_CHUNK_SIZE = 0x4000;
    static constexpr uint16_t CHR_ROM_CHUNK_SIZE = 0x2000;

    std::vector<uint8_t> prgRom;
    std::vector<uint8_t> chrRom;

    std::unique_ptr<Mapper> mapper;

    bool valid;

};

#endif // CARTRIDGE_HPP

