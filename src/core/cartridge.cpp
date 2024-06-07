#include "cartridge.hpp"

#include <fstream>
#include <filesystem>

Cartridge::Cartridge(){
    valid = false;
}

Cartridge::Cartridge(const std::string& filePath){
    valid = loadINESFile(filePath);
}

bool Cartridge::loadINESFile(const std::string& filePath){
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
    struct Header{
        std::array<char, 4> name;
        uint8_t prgRomChunks;
        uint8_t chrRomChunks;
        uint8_t flag6, flag7, flag8, flag9, flag10;
        std::array<char, 5> unused;
    };

    // TODO: add error message enum, change return depending on failure type
    if(std::filesystem::path(filePath).extension() != ".nes"){
        // File extension is incorrect
        return false;
    }

    std::ifstream file(filePath);
    if(!file){
        // File does not exist
        return false;
    }

    Header header;
    file.read((char*)&header, sizeof(Header));
    if(!file){
        // File is too small to contain header
        return false;
    }
    
    static constexpr std::array<char, 4> correctName = {'N', 'E', 'S', '\x1A'};
    if(header.name != correctName){
        // Name given in file is incorrect
        return false;
    }

    uint8_t mapperIdLo = (header.flag6 >> 4) & 0x0F;
    uint8_t mapperIdHi = (header.flag7 >> 4) & 0x0F;
    uint8_t mapperId = (mapperIdHi << 4) | mapperIdLo;

    bool isTrainer = (header.flag6 >> 2) & 1;

    if(isTrainer){
        static constexpr uint16_t TRAINER_SIZE = 0x200;

        // TODO: Maybe we should put the trainer data somewhere...
        file.ignore(TRAINER_SIZE);
        
        if(!file){
            // File is too small to contain trainer
            return false;
        }
    }

    // TODO: parse more fields
    // Yes:
    //      iNES version
    // Maybe:
    //      Nametable arrangement
    //      Alternate nametable layout

    mapper = Mapper::createMapper(mapperId, header.prgRomChunks, header.chrRomChunks);
    if(mapper == nullptr){
        // The chosen mapper has not been implemented yet
        return false;
    }

    prgRom.resize(header.prgRomChunks * PRG_ROM_CHUNK_SIZE);
    file.read((char*)prgRom.data(), prgRom.size() * sizeof(uint8_t));
    if(!file){
        // File is too small to contain PRG ROM
        return false;
    }

    chrRom.resize(header.chrRomChunks * CHR_ROM_CHUNK_SIZE);
    file.read((char*)chrRom.data(), chrRom.size() * sizeof(uint8_t));
    if(!file){
        // File is too small to contain CHR ROM
        return false;
    }

    return true;
}

std::optional<uint8_t> Cartridge::readFromPRG(uint16_t preMappedAddr){
    std::optional<uint32_t> mappedAddr = mapper->mapToPRGRead(preMappedAddr);
    if(mappedAddr.has_value()){
        return prgRom[mappedAddr.value()];
    }
    else{
        return std::nullopt;
    }
}
void Cartridge::writeToPRG(uint16_t preMappedAddr, uint8_t data){
    std::optional<uint32_t> mappedAddr = mapper->mapToPRGWrite(preMappedAddr);
    if(mappedAddr.has_value()){
        prgRom[mappedAddr.value()] = data;
    }
}
std::optional<uint8_t> Cartridge::readFromCHR(uint16_t preMappedAddr){
    std::optional<uint32_t> mappedAddr = mapper->mapToCHRRead(preMappedAddr);
    if(mappedAddr.has_value()){
        return chrRom[mappedAddr.value()];
    }
    else{
        return std::nullopt;
    }
}
void Cartridge::writeToCHR(uint16_t preMappedAddr, uint8_t data){
    std::optional<uint32_t> mappedAddr = mapper->mapToCHRWrite(preMappedAddr);
    if(mappedAddr.has_value()){
        chrRom[mappedAddr.value()] = data;
    }
}

bool Cartridge::isValid() const{
    return valid;
}