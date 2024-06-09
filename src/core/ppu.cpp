#include "ppu.hpp"

const std::array<uint32_t, PPU::NUM_SCREEN_COLORS> PPU::SCREEN_COLORS = initScreenColors();

// Screen colors taken from https://www.nesdev.org/wiki/PPU_palettes
std::array<uint32_t, PPU::NUM_SCREEN_COLORS> PPU::initScreenColors(){
    // Colors are 32 bit integers with 8 bits each for a = 0xFF, r, g, b
    std::array<uint32_t, NUM_SCREEN_COLORS> screenColors = {
        0xFF626262, 0xFF001FB2, 0xFF2404C8, 0xFF5200B2, 0xFF730076, 0xFF800024, 0xFF730B00, 0xFF522800, 0xFF244400, 0xFF005700, 0xFF005C00, 0xFF005324, 0xFF003C76, 0xFF000000, 0xFF000000, 0xFF000000, 
        0xFFABABAB, 0xFF0D57FF, 0xFF4B30FF, 0xFF8A13FF, 0xFFBC08D6, 0xFFD21269, 0xFFC72E00, 0xFF9D5400, 0xFF607B00, 0xFF209800, 0xFF00A300, 0xFF009942, 0xFF007DB4, 0xFF000000, 0xFF000000, 0xFF000000, 
        0xFFFFFFFF, 0xFF53AEFF, 0xFF9085FF, 0xFFD365FF, 0xFFFF57FF, 0xFFFF5DCF, 0xFFFF7757, 0xFFFA9E00, 0xFFBDC700, 0xFF7AE700, 0xFF43F611, 0xFF26EF7E, 0xFF2CD5F6, 0xFF4E4E4E, 0xFF000000, 0xFF000000, 
        0xFFFFFFFF, 0xFFB6E1FF, 0xFFCED1FF, 0xFFE9C3FF, 0xFFFFBCFF, 0xFFFFBDF4, 0xFFFFC6C3, 0xFFFFD59A, 0xFFE9E681, 0xFFCEF481, 0xFFB6FB9A, 0xFFA9FAC3, 0xFFA9F0F4, 0xFFB8B8B8, 0xFF000000, 0xFF000000,
    };
    return screenColors;
}

void PPU::setBus(Bus* bus){
    this->bus = bus;
}

void PPU::setCartridge(std::shared_ptr<Cartridge>& cartridge){
    this->cartridge = cartridge;
}

void PPU::initPPU(){
    control = 0;
    mask = 0;
    ppuBusData = 0;
    vramAddress = 0;
    workingAddress = 0;
    palleteRam = {};
}

uint8_t PPU::view(uint8_t ppuRegister) const{
    if(ppuRegister == PPUSTATUS){
        return status.toByte();
    }
    else if(ppuRegister == OAMDATA){
        return 0;
    }
    else if(ppuRegister == PPUDATA){
        uint8_t data = ppuBusData;

        // pallete addresses get returned immediately
        if(PALLETE_RAM_RANGE.contains(vramAddress & 0x3FFF)){
            data = ppuView(vramAddress & 0x3FFF);
        }

        return data;
    }
    else{
        return 0;
    }
}

uint8_t PPU::read(uint8_t ppuRegister){
    if(ppuRegister == PPUSTATUS){
        status.vBlankStarted = 1; // TODO: temp to get pattern table to render

        uint8_t data = status.toByte();

        status.vBlankStarted = 0;
        addressLatch = 0;
        return data;
    }
    else if(ppuRegister == OAMDATA){
        return 0;
    }
    else if(ppuRegister == PPUDATA){
        uint8_t data = ppuBusData;
        
        ppuBusData = ppuRead(vramAddress & 0x3FFF);
        
        // open bus is the bottom 5 bits of the bus
        status.openBus = ppuBusData & 0x1F;

        // pallete addresses get returned immediately
        if(PALLETE_RAM_RANGE.contains(vramAddress & 0x3FFF)){
            data = ppuBusData;
        }
        
        vramAddress += (control.vramAddressIncrement ? 32 : 1);

        return data;
    }
    else{
        return 0;
    }
}

void PPU::write(uint8_t ppuRegister, uint8_t value){
    if(ppuRegister == PPUCTRL){
        control.setFromByte(value);
    }
    else if(ppuRegister == PPUMASK){
        mask.setFromByte(value);
    }
    else if(ppuRegister == OAMADDR){

    }
    else if(ppuRegister == OAMDATA){

    }
    else if(ppuRegister == PPUSCROLL){

    }
    else if(ppuRegister == PPUADDR){
        if(addressLatch == 0){ 
            workingAddress &= 0x00FF;
            workingAddress |= value << 8;
        }
        else{
            workingAddress &= 0xFF00;
            workingAddress |= value;

            vramAddress = workingAddress;
        }
        addressLatch ^= 1;
    }
    else if(ppuRegister == PPUDATA){
        // TODO: Fix this. The below code should not be here.
        // But because of the way the code is structured, calls to write also call read.
        // So vramAddress already gets incremented by 1 before we write.
        // So we need to subtract before we use it, then add it back. 
        // vramAddress -= (control.vramAddressIncrement ? 32 : 1);
        
        ppuWrite(vramAddress & 0x3FFF, value);
        vramAddress += (control.vramAddressIncrement ? 32 : 1);
    }
}

uint8_t PPU::getPalleteRamIndex(uint16_t address) const{
    address &= 0x1F;
    
    // address mirroring to handle background tiles
    if(address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1C){
        address &= 0x0F;
    }

    return address;
}

uint8_t PPU::ppuView(uint16_t address) const{
    if(CARTRIDGE_RANGE.contains(address)){
        std::optional<uint8_t> data = cartridge->viewCHR(address);
        return data.value_or(0);
    }
    else if(PALLETE_RAM_RANGE.contains(address)){
        return palleteRam[getPalleteRamIndex(address)];
    }
    else{
        return 0;
    }
}

uint8_t PPU::ppuRead(uint16_t address){
    if(CARTRIDGE_RANGE.contains(address)){
        std::optional<uint8_t> data = cartridge->readFromCHR(address);
        return data.value_or(0);
    }
    else if(PALLETE_RAM_RANGE.contains(address)){
        return palleteRam[getPalleteRamIndex(address)];
    }
    else{
        return 0;
    }
}

void PPU::ppuWrite(uint16_t address, uint8_t value){
    if(CARTRIDGE_RANGE.contains(address)){
        cartridge->writeToCHR(address, value);
    }
    else if(PALLETE_RAM_RANGE.contains(address)){
        palleteRam[getPalleteRamIndex(address)] = value;
    }
    else{
        // ...
    }
}

// TODO: maybe optimize to return the data before the pallete is applied
PPU::PatternTable PPU::getPatternTable(bool tableNumber, uint8_t palleteNumber) const{
    PatternTable table{};
    for(int tileRow = 0; tileRow < PATTERN_TABLE_NUM_TILES; tileRow++){
        for(int tileCol = 0; tileCol < PATTERN_TABLE_NUM_TILES; tileCol++){
            uint16_t tableOffset = PATTERN_TABLE_TILE_BYTES * (PATTERN_TABLE_NUM_TILES * tileRow + tileCol);
            
            for(int spriteRow = 0; spriteRow < PATTERN_TABLE_TILE_SIZE; spriteRow++){
                uint8_t loBits = ppuView(PATTERN_TABLE_TOTAL_BYTES * tableNumber + tableOffset + spriteRow);
                uint8_t hiBits = ppuView(PATTERN_TABLE_TOTAL_BYTES * tableNumber + tableOffset + spriteRow + 0x8);
             
                for(int spriteCol = 0; spriteCol < PATTERN_TABLE_TILE_SIZE; spriteCol++){
                    bool currentLoBit = (loBits >> spriteCol) & 1;
                    bool currentHiBit = (hiBits >> spriteCol) & 1;
                    uint8_t palleteIndex = (currentHiBit << 1) | currentLoBit;
                    
                    uint16_t pixelRow = PATTERN_TABLE_TILE_SIZE * tileRow + spriteRow;
                    uint16_t pixelCol = PATTERN_TABLE_TILE_SIZE * tileCol + PATTERN_TABLE_TILE_SIZE - 1 - spriteCol;

                    uint16_t addr = PALLETE_RAM_RANGE.lo + (palleteNumber << 2) + palleteIndex;
                    table[pixelRow][pixelCol] = SCREEN_COLORS[ppuView(addr) & 0x3F];
                }
            }
        }
    }
    return table;
}

void PPU::executeCycle(){
    // TODO: eventually implement scanlines, set vertical blank, and handle NMI
    // if(control.nmiEnabled){
    //     bus->nmiRequest = true;
    // }
}

std::array<uint32_t, 0x20> PPU::getPalleteRamColors() const{
    std::array<uint32_t, 0x20> result;
    for(int i = 0; i < 0x20; i++){
        uint8_t index = getPalleteRamIndex(i);
        result[i] = SCREEN_COLORS[palleteRam[index] & 0x3F];
    }
    return result;
}

// Helper struct function definitions
PPU::Control::Control(uint8_t data){
    setFromByte(data);
}

uint8_t PPU::Control::toByte() const{
    uint8_t data = 
        baseNameTableAddress |
        (vramAddressIncrement << 2) |
        (spritePatternTable << 3) |
        (backgroundPatternTable << 4) |
        (spriteSize << 5) | 
        (ppuSelect << 6) |
        (nmiEnabled << 7);
    
    return data;
}

void PPU::Control::setFromByte(uint8_t data){
    baseNameTableAddress = data & 0b11;
    vramAddressIncrement = (data >> 2) & 1;
    spritePatternTable = (data >> 3) & 1;
    backgroundPatternTable = (data >> 4) & 1;
    spriteSize = (data >> 5) & 1;
    ppuSelect = (data >> 6) & 1;
    nmiEnabled = (data >> 7) & 1;
}


PPU::Mask::Mask(uint8_t value){
    setFromByte(value);
}

uint8_t PPU::Mask::toByte() const{
    uint8_t data = 
        greyscale |
        (showBackgroundLeft << 1) |
        (showSpritesLeft << 2) |
        (showBackground << 3) |
        (showSprites << 4) |
        (emphRed << 5) |
        (emphGreen << 6) |
        (emphBlue << 7);
    
    return data;
}

void PPU::Mask::setFromByte(uint8_t data) {
    greyscale = data & 1;
    showBackgroundLeft = (data >> 1) & 1;
    showSpritesLeft = (data >> 2) & 1;
    showBackground = (data >> 3) & 1;
    showSprites = (data >> 4) & 1;
    emphRed = (data >> 5) & 1;
    emphGreen = (data >> 6) & 1;
    emphBlue = (data >> 7) & 1;
}


PPU::Status::Status(uint8_t data){
    setFromByte(data);
}

uint8_t PPU::Status::toByte() const{
    uint8_t data = 
        openBus |
        (spriteOverflow << 5) |
        (sprite0Hit << 6) |
        (vBlankStarted << 7);
    
    return data;
}

void PPU::Status::setFromByte(uint8_t data){
    openBus = data & 0x1F;
    spriteOverflow = (data >> 5) & 1;
    sprite0Hit = (data >> 6) & 1;
    vBlankStarted = (data >> 7) & 1;
}