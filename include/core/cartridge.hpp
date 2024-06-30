#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include "core/mapper/mapper.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Cartridge {
public:
    Cartridge();
    Cartridge(const std::string& filePath);

    uint8_t viewPRG(uint16_t preMappedAddr) const;
    uint8_t readFromPRG(uint16_t preMappedAddr);
    void writeToPRG(uint16_t preMappedAddr, uint8_t data);

    uint8_t viewCHR(uint16_t preMappedAddr) const;
    uint8_t readFromCHR(uint16_t preMappedAddr);
    void writeToCHR(uint16_t preMappedAddr, uint8_t data);

    Mapper::MirrorMode getMirrorMode() const;

    enum class Code {
        SUCCESS,
        INCORRECT_EXTENSION,
        MISSING_FILE,
        MISSING_HEADER,
        INCORRECT_HEADER_NAME,
        MISSING_TRAINER,
        UNIMPLEMENTED_MAPPER,
        UNSUPPORTED_INES_VERSION,
        MISSING_PRG,
        MISSING_CHR
    };

    struct Status {
        Code code;
        std::string message;
    };

    Status getStatus() const;

    static constexpr uint16_t PRG_ROM_CHUNK_SIZE = 0x4000;
    static constexpr uint16_t CHR_ROM_CHUNK_SIZE = 0x2000;

private:
    Status status;
    Status loadINESFile(const std::string& filePath);

    std::vector<uint8_t> prgRom;
    std::vector<uint8_t> chrRom;
    std::unique_ptr<Mapper> mapper;
};

#endif // CARTRIDGE_HPP

