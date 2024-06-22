#include "cartridge.hpp"

#include <fstream>
#include <filesystem>

Cartridge::Cartridge() {
    status = MISSING_FILE;
}

Cartridge::Cartridge(const std::string& filePath) {
    status = loadINESFile(filePath);
}

Cartridge::Status Cartridge::loadINESFile(const std::string& filePath) {
    // iNES file format (https://www.nesdev.org/wiki/INES)
    // An iNES file consists of the following sections, in order:
    // Header (16 bytes)
    // Trainer, if present (0 or 512 bytes)
    // PRG ROM data (16384 * x bytes)
    // CHR ROM data, if present (8192 * y bytes)
    // PlayChoice INST-ROM, if present (0 or 8192 bytes)
    // PlayChoice PROM, if present (16 bytes Data, 16 bytes CounterOut) (this is often missing; see PC10 ROM-Images for details)

    // iNES header (https://www.nesdev.org/wiki/INES)
    // 0-3	Constant $4E $45 $53 $1A (ASCII "NES" followed by MS-DOS end-of-file)
    // 4	Size of PRG ROM in 16 KB units
    // 5	Size of CHR ROM in 8 KB units (value 0 means the board uses CHR RAM)
    // 6	Flags 6 – Mapper, mirroring, battery, trainer
    // 7	Flags 7 – Mapper, VS/Playchoice, NES 2.0
    // 8	Flags 8 – PRG-RAM size (rarely used extension)
    // 9	Flags 9 – TV system (rarely used extension)
    // 10	Flags 10 – TV system, PRG-RAM presence (unofficial, rarely used extension)
    // 11-15	Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15)
    struct Header {
        std::array<uint8_t, 4> name;
        uint8_t prgRomChunks;
        uint8_t chrRomChunks;
        uint8_t flag6, flag7, flag8, flag9, flag10;
        std::array<uint8_t, 5> unused;
    };

    auto readHeader = [](std::ifstream& file, Header& header) -> Status {
        std::array<uint8_t, 16> buffer;
        file.read((char*)buffer.data(), buffer.size() * sizeof(uint8_t));
        if (!file) {
            return MISSING_HEADER;
        }

        header.name = { buffer[0], buffer[1], buffer[2], buffer[3] };
        header.prgRomChunks = buffer[4];
        header.chrRomChunks = buffer[5];
        header.flag6 = buffer[6];
        header.flag7 = buffer[7];
        header.flag8 = buffer[8];
        header.flag9 = buffer[9];
        header.flag10 = buffer[10];
        header.unused = { buffer[11], buffer[12], buffer[13], buffer[14], buffer[15] };

        return SUCCESS;
    };

    if (std::filesystem::path(filePath).extension() != ".nes") {
        return INCORRECT_EXTENSION;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return MISSING_FILE;
    }

    Header header;
    readHeader(file, header);
    if (!file) {
        return MISSING_HEADER;
    }

    static constexpr std::array<uint8_t, 4> correctName = { 'N', 'E', 'S', '\x1A' };
    if (header.name != correctName) {
        return INCORRECT_HEADER_NAME;
    }

    bool isTrainer = (header.flag6 >> 2) & 0x1;

    if (isTrainer) {
        static constexpr uint16_t TRAINER_SIZE = 0x200;

        // TODO: Maybe we should put the trainer data somewhere...
        file.ignore(TRAINER_SIZE);

        if (!file) {
            return MISSING_TRAINER;
        }
    }

    mirrorMode = (MirrorMode)(header.flag6 & 1);

    uint8_t mapperIdLo = (header.flag6 >> 4) & 0xF;
    uint8_t mapperIdHi = (header.flag7 >> 4) & 0xF;
    uint8_t mapperId = (mapperIdHi << 4) | mapperIdLo;

    mapper = Mapper::createMapper(mapperId, header.prgRomChunks, header.chrRomChunks);
    if (mapper == nullptr) {
        return UNIMPLEMENTED_MAPPER;
    }

    // TODO: Parse iNES version
    // uint8_t iNESVersion = (((header.flag7 >> 2) & 0x3) == 2) ? 2 : 1;
    // if(iNESVersion != 1){
    //     return UNSUPPORTED_INES_VERSION;
    // }

    // TODO: Parse alternative nametable layout

    prgRom.resize(header.prgRomChunks * PRG_ROM_CHUNK_SIZE);
    file.read((char*)prgRom.data(), prgRom.size() * sizeof(uint8_t));
    if (!file) {
        return MISSING_PRG;
    }

    // For iNES 1.0 we assume that a value of 0 for char rom chunks means we have 1 chunk of CHR RAM.
    // In iNES 2.0 the size is specified
    // TODO: Add iNES 2.0 support
    if (header.chrRomChunks == 0) {
        chrRom.resize(1 * CHR_ROM_CHUNK_SIZE);
        std::fill(chrRom.begin(), chrRom.end(), 0);
    }
    else {
        chrRom.resize(header.chrRomChunks * CHR_ROM_CHUNK_SIZE);
        file.read((char*)chrRom.data(), chrRom.size() * sizeof(uint8_t));
    }

    if (!file) {
        return MISSING_CHR;
    }

    return SUCCESS;
}

const std::string Cartridge::getMessage(Status status) {
    switch (status) {
    case INCORRECT_EXTENSION:       return "Requested file has an incorrect extension (.nes is required)";
    case MISSING_FILE:              return "No file was requested, or requested file does not exist";
    case MISSING_HEADER:            return "File does not contain an iNES header.";
    case INCORRECT_HEADER_NAME:     return "File header contains incorrect name.";
    case MISSING_TRAINER:           return "Trainer data should be present, but was not found or incomplete.";
    case UNIMPLEMENTED_MAPPER:      return "The requested mapper is currently not supported.";
    case UNSUPPORTED_INES_VERSION:  return "The requested iNES version is currently not supported.";
    case MISSING_PRG:               return "Program data not found.";
    case MISSING_CHR:               return "Character data not found.";
    case SUCCESS: default:          return "";
    }
}

Cartridge::MirrorMode Cartridge::getMirrorMode() const {
    return mirrorMode;
}

std::optional<uint8_t> Cartridge::viewPRG(uint16_t preMappedAddr) const {
    std::optional<uint32_t> mappedAddr = mapper->mapToPRGView(preMappedAddr);
    if (mappedAddr.has_value()) {
        return prgRom[mappedAddr.value()];
    }
    else {
        return std::nullopt;
    }
}

std::optional<uint8_t> Cartridge::readFromPRG(uint16_t preMappedAddr) {
    std::optional<uint32_t> mappedAddr = mapper->mapToPRGRead(preMappedAddr);
    if (mappedAddr.has_value()) {
        return prgRom[mappedAddr.value()];
    }
    else {
        return std::nullopt;
    }
}
void Cartridge::writeToPRG(uint16_t preMappedAddr, uint8_t data) {
    std::optional<uint32_t> mappedAddr = mapper->mapToPRGWrite(preMappedAddr);
    if (mappedAddr.has_value()) {
        prgRom[mappedAddr.value()] = data;
    }
}


std::optional<uint8_t> Cartridge::viewCHR(uint16_t preMappedAddr) const {
    std::optional<uint32_t> mappedAddr = mapper->mapToCHRView(preMappedAddr);
    if (mappedAddr.has_value()) {
        return chrRom[mappedAddr.value()];
    }
    else {
        return std::nullopt;
    }
}

std::optional<uint8_t> Cartridge::readFromCHR(uint16_t preMappedAddr) {
    std::optional<uint32_t> mappedAddr = mapper->mapToCHRRead(preMappedAddr);
    if (mappedAddr.has_value()) {
        return chrRom[mappedAddr.value()];
    }
    else {
        return std::nullopt;
    }
}
void Cartridge::writeToCHR(uint16_t preMappedAddr, uint8_t data) {
    std::optional<uint32_t> mappedAddr = mapper->mapToCHRWrite(preMappedAddr);
    if (mappedAddr.has_value()) {
        chrRom[mappedAddr.value()] = data;
    }
}

Cartridge::Status Cartridge::getStatus() const {
    return status;
}