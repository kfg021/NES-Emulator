#include "core/cartridge.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

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
        uint8_t prgChunks;
        uint8_t chrChunks;
        uint8_t flag6, flag7, flag8, flag9, flag10;
        std::array<uint8_t, 5> unused;
    };

    if (std::filesystem::path(filePath).extension() != ".nes") {
        return { Code::INCORRECT_EXTENSION, "Requested file (" + filePath + ") has an incorrect extension (.nes is required)." };
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return { Code::MISSING_FILE, "Requested file (" + filePath + ") does not exist." };
    }

    auto readHeader = [](std::ifstream& file, Header& header) -> void {
        std::array<uint8_t, 16> buffer;
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size() * sizeof(uint8_t));
        if (!file) {
            return;
        }

        header.name = { buffer[0], buffer[1], buffer[2], buffer[3] };
        header.prgChunks = buffer[4];
        header.chrChunks = buffer[5];
        header.flag6 = buffer[6];
        header.flag7 = buffer[7];
        header.flag8 = buffer[8];
        header.flag9 = buffer[9];
        header.flag10 = buffer[10];
        header.unused = { buffer[11], buffer[12], buffer[13], buffer[14], buffer[15] };
    };

    Header header;
    readHeader(file, header);
    if (!file) {
        return { Code::MISSING_HEADER, "iNES header missing or incomplete." };
    }

    static constexpr std::array<uint8_t, 4> correctName = { 'N', 'E', 'S', '\x1A' };
    if (header.name != correctName) {
        return { Code::INCORRECT_HEADER_NAME, "File header contains incorrect name." };
    }

    bool isTrainer = (header.flag6 >> 2) & 0x1;
    if (isTrainer) {
        static constexpr uint16_t TRAINER_SIZE = 0x200;

        // TODO: Maybe we should put the trainer data somewhere...
        file.ignore(TRAINER_SIZE);

        if (!file) {
            return { Code::MISSING_TRAINER, "Trainer data should be present, but missing or incomplete." };
        }
    }

    uint8_t iNESVersion = (((header.flag7 >> 2) & 0x3) == 2) ? 2 : 1;

    uint16_t mapperId;
    if (iNESVersion == 1) {
        // In an older version of the iNES file format ("Archaic iNES"), bytes 7-15 were ignored and sometimes contain garbage information, giving an incorrect value for the mapper.
        // So we need a way to determine if the file uses this old format.
        // (From https://www.nesdev.org/wiki/INES): A general rule of thumb: if the last 4 bytes are not all zero, and the header is not marked for NES 2.0 format, an emulator should either mask off the upper 4 bits of the mapper number or simply refuse to load the ROM.
        bool last4Bytes0 = (header.unused[1] | header.unused[2] | header.unused[3] | header.unused[4]) == 0;
        uint8_t mapperIdLo = (header.flag6 >> 4) & 0xF;
        uint8_t mapperIdHi = last4Bytes0 ? (header.flag7 >> 4) & 0xF : 0;
        mapperId = (mapperIdHi << 4) | mapperIdLo;
    }
    else { // if (iNESVersion == 2)
        uint8_t mapperIdLo = (header.flag6 >> 4) & 0xF;
        uint8_t mapperIdMid = (header.flag7 >> 4) & 0xF;
        uint8_t mapperIdHi = header.flag8 & 0xF;
        mapperId = (mapperIdHi << 8) | (mapperIdMid << 4) | mapperIdLo;
    }

    bool alternativeNametableLayout = (header.flag6 >> 3) & 1;

    // TODO: Parse iNES 2.0 fields

    std::vector<uint8_t> prg(header.prgChunks * Mapper::PRG_ROM_CHUNK_SIZE);
    file.read(reinterpret_cast<char*>(prg.data()), prg.size() * sizeof(uint8_t));
    if (!file) {
        return { Code::MISSING_PRG, "Program data missing or incomplete." };
    }

    // For iNES 1.0 we assume that a value of 0 for char rom chunks means we have 1 chunk of CHR RAM.
    // In iNES 2.0 the size is specified
    // TODO: Add iNES 2.0 support
    std::vector<uint8_t> chr;
    if (header.chrChunks == 0) {
        chr = std::vector<uint8_t>(1 * Mapper::CHR_ROM_CHUNK_SIZE, 0);
    }
    else {
        chr = std::vector<uint8_t>(header.chrChunks * Mapper::CHR_ROM_CHUNK_SIZE);
        file.read(reinterpret_cast<char*>(chr.data()), chr.size() * sizeof(uint8_t));
    }
    if (!file) {
        return { Code::MISSING_CHR, "Character data missing or incomplete." };
    }

    bool mirrorModeId = header.flag6 & 1;
    bool hasBatteryBackedPrgRam = (header.flag6 >> 1) & 1;

    Mapper::Config config = {
        mapperId,
        header.prgChunks,
        header.chrChunks,
        static_cast<Mapper::MirrorMode>(mirrorModeId),
        hasBatteryBackedPrgRam,
        alternativeNametableLayout
    };
    mapper = Mapper::createMapper(config, prg, chr);
    if (mapper == nullptr) {
        return { Code::UNIMPLEMENTED_MAPPER, "The requested mapper (" + std::to_string(mapperId) + ") is currently not supported." };
    }

    return { Code::SUCCESS, "" };
}

Mapper::MirrorMode Cartridge::getMirrorMode() const {
    return mapper->getMirrorMode();
}

Cartridge::Status Cartridge::getStatus() const {
    return status;
}