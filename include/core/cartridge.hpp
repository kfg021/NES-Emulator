#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include "core/mapper/mapper.hpp"

#include "util/util.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Cartridge {
public:
    Cartridge();
    Cartridge(const std::string& filePath);

    // uint8_t viewPRG(uint16_t preMappedAddr) const;
    // uint8_t readFromPRG(uint16_t preMappedAddr);
    // void writeToPRG(uint16_t preMappedAddr, uint8_t data);

    // uint8_t viewCHR(uint16_t preMappedAddr) const;
    // uint8_t readFromCHR(uint16_t preMappedAddr);
    // void writeToCHR(uint16_t preMappedAddr, uint8_t data);

    Mapper::MirrorMode getMirrorMode() const;

    enum class Code {
        SUCCESS,
        INCORRECT_EXTENSION,
        MISSING_FILE,
        MISSING_HEADER,
        INCORRECT_HEADER_NAME,
        MISSING_TRAINER,
        UNIMPLEMENTED_MAPPER,
        MISSING_PRG,
        MISSING_CHR
    };

    struct Status {
        Code code;
        std::string message;
    };

    Status getStatus() const;

    std::unique_ptr<Mapper> mapper;

private:
    Status status;
    Status loadINESFile(const std::string& filePath);
};

#endif // CARTRIDGE_HPP

