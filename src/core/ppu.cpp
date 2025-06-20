#include "core/ppu.hpp"

#include "core/mapper/mapper4.hpp"

PPU::PPU(Cartridge& cartridge) : cartridge(cartridge) {
    workingDisplay = std::make_unique<Display>();
    finishedDisplay = std::make_unique<Display>();

    resetPPU();
}

void PPU::resetPPU() {
    control.data = 0;
    mask.data = 0;
    status.data = 0;

    ppuBusData = 0;
    vramAddress.data = 0;
    temporaryVramAddress.data = 0;
    addressLatch = 0;
    palleteRam = {};

    scanline = 0;
    cycle = 0;
    oddFrame = false;

    patternTableLoShifter = 0;
    patternTableHiShifter = 0;
    attributeTableLoShifter = 0;
    attributeTableHiShifter = 0;
    nextNameTableByte = 0;
    nextPatternTableLo = 0;
    nextPatternTableHi = 0;
    nextAttributeTableLo = 0;
    nextAttributeTableHi = 0;
    fineX = 0;

    nameTable = {};

    workingDisplay->fill(std::array<uint32_t, 256>{});
    finishedDisplay->fill(std::array<uint32_t, 256>{});

    // Reset the OAM buffer to 0xFF so that sprites start off the screen
    oamBuffer.fill(0xFF);

    oamAddress = 0;

    currentScanlineSprites.reserve(MAX_SPRITES);
    sprite0OnCurrentScanline = false;

    frameReadyFlag = false;

    nmiRequest = false;
    irqRequest = false;

    nmiDelayCounter = 0;
}

bool PPU::nmiRequested() const {
    return nmiRequest;
}

void PPU::clearNMIRequest() {
    nmiRequest = false;
}

bool PPU::irqRequested() const {
    return irqRequest;
}

uint8_t PPU::view(uint8_t ppuRegister) const {
    switch (static_cast<Register>(ppuRegister)) {
        case Register::PPUSTATUS:
            return status.data;

        case Register::OAMDATA:
            return oamBuffer[oamAddress];

        case Register::PPUDATA: {
            uint8_t data = ppuBusData;

            // Pallete addresses get returned immediately
            if (PALLETE_RAM_RANGE.contains(vramAddress.data & 0x3FFF)) {
                data = ppuView(vramAddress.data & 0x3FFF);
            }
            return data;
        }

        default:
            return 0;
    }
}

uint8_t PPU::read(uint8_t ppuRegister) {
    switch (static_cast<Register>(ppuRegister)) {
        case Register::PPUSTATUS: {
            uint8_t data = status.data;
            status.vBlankStarted = 0;
            addressLatch = 0;
            return data;
        }

        case Register::OAMDATA:
            return oamBuffer[oamAddress];

        case Register::PPUDATA: {
            uint8_t data = ppuBusData;

            ppuBusData = ppuRead(vramAddress.data & 0x3FFF);

            // Open bus is the bottom 5 bits of the bus
            status.openBus = ppuBusData & 0x1F;

            // Pallete addresses get returned immediately
            if (PALLETE_RAM_RANGE.contains(vramAddress.data & 0x3FFF)) {
                data = ppuBusData;
            }

            vramAddress.data += (control.vramAddressIncrement ? 32 : 1);

            return data;
        }

        default:
            return 0;
    }
}

void PPU::write(uint8_t ppuRegister, uint8_t value) {
    switch (static_cast<Register>(ppuRegister)) {
        case Register::PPUCTRL: {
            bool oldNmiFlag = control.nmiEnabled;

            control.data = value;

            bool newNmiFlag = control.nmiEnabled;

            // From (https://www.nesdev.org/wiki/PPU_registers#PPUCTRL): 
            // If the PPU is currently in vertical blank, and the PPUSTATUS ($2002) vblank flag is still set (1), changing the NMI flag in bit 7 of $2000 from 0 to 1 will immediately generate an NMI. 
            if (status.vBlankStarted && !oldNmiFlag && newNmiFlag) {
                nmiDelayCounter = NMI_DELAY_TIME;
            }

            temporaryVramAddress.nametableX = control.nametableX;
            temporaryVramAddress.nametableY = control.nametableY;
            break;
        }

        case Register::PPUMASK:
            mask.data = value;
            break;

        case Register::OAMADDR:
            oamAddress = value;
            break;

        case Register::OAMDATA:
            oamBuffer[oamAddress] = value;
            break;

        case Register::PPUSCROLL:
            if (addressLatch == 0) {
                fineX = value & 0x7;
                temporaryVramAddress.coarseX = value >> 3;
            }
            else {
                temporaryVramAddress.fineY = value & 0x7;
                temporaryVramAddress.coarseY = value >> 3;
            }
            addressLatch ^= 1;
            break;

        case Register::PPUADDR:
            if (addressLatch == 0) {
                temporaryVramAddress.data &= 0x00FF;
                temporaryVramAddress.data |= (value << 8);
            }
            else {
                temporaryVramAddress.data &= 0xFF00;
                temporaryVramAddress.data |= value;

                vramAddress.data = temporaryVramAddress.data;
            }
            addressLatch ^= 1;
            break;

        case Register::PPUDATA:
            ppuWrite(vramAddress.data & 0x3FFF, value);
            vramAddress.data += (control.vramAddressIncrement ? 32 : 1);
            break;

        default:
            break;
    }
}

// Address mirroring to handle background tiles
// When we are reading from the pallete ram, we consider every 4th tile to be equivalent to the background pixel
// But, it turns out that these pallete ram indices can actually be set (setting them just has no effect)
uint8_t PPU::getPalleteRamIndexRead(uint16_t address) const {
    address &= 0x1F;

    if ((address & 0x3) == 0) {
        address = 0;
    }

    return address;
}

uint8_t PPU::getPalleteRamIndexWrite(uint16_t address) const {
    address &= 0x1F;

    if (address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1C) {
        address &= 0x0F;
    }

    return address;
}

uint8_t PPU::viewPalleteRam(uint16_t address) const {
    uint8_t data = palleteRam[getPalleteRamIndexRead(address)] & 0x3F;
    if (mask.greyscale) {
        data &= 0x30;
    }
    return data;
}

uint16_t PPU::getNameTableIndex(uint16_t address) const {
    Mapper::MirrorMode mirrorMode = cartridge.mapper->getMirrorMode();

    // Nametable mirroring maps each of the four quadrants of the address space [0x000 - 0xFFF] to either nametable A or nametable B
    uint8_t quadrant = (address >> 10) & 0x3;

    bool isNameTableB;
    switch (mirrorMode) {
        case Mapper::MirrorMode::HORIZONTAL:
            isNameTableB = (quadrant >> 1) & 1;
            break;
        case Mapper::MirrorMode::VERTICAL:
            isNameTableB = quadrant & 1;
            break;
        case Mapper::MirrorMode::ONE_SCREEN_LOWER_BANK:
            isNameTableB = false;
            break;
        default: /*Mapper::MirrorMode::ONE_SCREEN_UPPER_BANK:*/
            isNameTableB = true;
    }

    // Bits 10 and 11 determine the nametable, and we can modify them to map any address onto a specific nametable.
    // Setting {bit10, bit11} = {0, 0} maps to nametable A
    // Setting {bit10, bit11} = {0, 1} maps to nametable B
    // Thus, ANDing the address with 0x3FF maps to nametable A, and ANDing the address with 0x3FF and then ORing with 0x400 maps to nametable B.
    address &= 0x3FF;
    if (isNameTableB) address |= 0x400;

    return address;
}

uint8_t PPU::viewNameTable(uint16_t address) const {
    if (cartridge.mapper->getMirrorMode() != Mapper::MirrorMode::FOUR_SCREEN) {
        return nameTable[getNameTableIndex(address)];
    }
    else {
        // Mapper handles nametables in 4 screen mode
        return cartridge.mapper->mapCHRView(address);
    }
}

uint8_t PPU::readNameTable(uint16_t address) {
    if (cartridge.mapper->getMirrorMode() != Mapper::MirrorMode::FOUR_SCREEN) {
        return nameTable[getNameTableIndex(address)];
    }
    else {
        // Mapper handles nametables in 4 screen mode
        return cartridge.mapper->mapCHRRead(address);
    }
}

uint8_t PPU::ppuView(uint16_t address) const {
    if (PATTERN_TABLE_RANGE.contains(address)) {
        return cartridge.mapper->mapCHRView(address);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        return viewNameTable(address);
    }
    else if (PALLETE_RAM_RANGE.contains(address)) {
        return viewPalleteRam(address);
    }
    else {
        return 0;
    }
}

uint8_t PPU::ppuRead(uint16_t address) {
    if (PATTERN_TABLE_RANGE.contains(address)) {
        return cartridge.mapper->mapCHRRead(address);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        return readNameTable(address);
    }
    else if (PALLETE_RAM_RANGE.contains(address)) {
        return viewPalleteRam(address);
    }
    else {
        return 0;
    }
}

void PPU::ppuWrite(uint16_t address, uint8_t value) {
    if (PATTERN_TABLE_RANGE.contains(address)) {
        cartridge.mapper->mapCHRWrite(address, value);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        if (cartridge.mapper->getMirrorMode() != Mapper::MirrorMode::FOUR_SCREEN) {
            nameTable[getNameTableIndex(address)] = value;
        }
        else {
            // Mapper handles nametables in 4 screen mode
            cartridge.mapper->mapCHRWrite(address, value);
        }
    }
    else if (PALLETE_RAM_RANGE.contains(address)) {
        palleteRam[getPalleteRamIndexWrite(address)] = value;
    }
}

std::unique_ptr<PPU::PatternTables> PPU::getPatternTables(uint8_t backgroundPalleteNumber, uint8_t spritePalleteNumber) const {
    std::unique_ptr<PPU::PatternTables> tables = std::make_unique<PPU::PatternTables>();

    for (int i = 0; i < 2; i++) {
        bool isBackground = !i;
        bool tableNumber = isBackground ? control.backgroundPatternTable : control.spritePatternTable;
        uint8_t palleteNumber = isBackground ? backgroundPalleteNumber : spritePalleteNumber;
        PatternTable& table = isBackground ? tables->backgroundPatternTable : tables->spritePatternTable;

        for (int tileRow = 0; tileRow < PATTERN_TABLE_NUM_TILES; tileRow++) {
            for (int tileCol = 0; tileCol < PATTERN_TABLE_NUM_TILES; tileCol++) {
                uint16_t tableOffset = PATTERN_TABLE_TILE_BYTES * (PATTERN_TABLE_NUM_TILES * tileRow + tileCol);

                for (int spriteRow = 0; spriteRow < PATTERN_TABLE_TILE_SIZE; spriteRow++) {
                    uint8_t loBits = cartridge.mapper->mapCHRView(PATTERN_TABLE_TOTAL_BYTES * tableNumber + tableOffset + spriteRow);
                    uint8_t hiBits = cartridge.mapper->mapCHRView(PATTERN_TABLE_TOTAL_BYTES * tableNumber + tableOffset + spriteRow + 0x8);

                    for (int spriteCol = 0; spriteCol < PATTERN_TABLE_TILE_SIZE; spriteCol++) {
                        bool currentLoBit = (loBits >> spriteCol) & 1;
                        bool currentHiBit = (hiBits >> spriteCol) & 1;
                        uint8_t palleteIndex = ((!isBackground) << 4) | (currentHiBit << 1) | static_cast<uint8_t>(currentLoBit);

                        uint16_t pixelRow = PATTERN_TABLE_TILE_SIZE * tileRow + spriteRow;
                        uint16_t pixelCol = PATTERN_TABLE_TILE_SIZE * tileCol + PATTERN_TABLE_TILE_SIZE - 1 - spriteCol;

                        uint16_t addr = getPalleteRamAddress(palleteIndex, palleteNumber);
                        table[pixelRow][pixelCol] = SCREEN_COLORS[viewPalleteRam(addr) & 0x3F];
                    }
                }
            }
        }
    }

    return tables;
}

void PPU::executeCycle() {
    if (nmiDelayCounter > 0) {
        nmiDelayCounter--;
        if (nmiDelayCounter == 0) {
            nmiRequest = true;
        }
    }

    if (scanline == -1) {
        preRenderScanline();
    }
    else if (scanline >= 0 && scanline <= 239) {
        visibleScanlines();
    }
    else if (scanline == 240) {
        // Do nothing on this scanline
    }
    else { // if (scanline >= 241 && scanline <= 260) {
        verticalBlankScanlines();
    }

    incrementCycle();
}

void PPU::handleMapper4IRQ() {
    if (cartridge.mapper->config.id == 4) {
        // We can static_cast instead of dynamic_cast because we explicitly checked id
        Mapper4* mapper4 = static_cast<Mapper4*>(cartridge.mapper.get());
        mapper4->clockIRQTimer();
        irqRequest = mapper4->irqRequested();
    }
}

void PPU::preRenderScanline() {
    if (cycle == 1) {
        status.vBlankStarted = 0;
        status.sprite0Hit = 0;
        status.spriteOverflow = 0;
    }
    else if (cycle >= 280 && cycle <= 304) {
        if (isRenderingEnabled()) {
            vramAddress.fineY = static_cast<uint16_t>(temporaryVramAddress.fineY);
            vramAddress.nametableY = static_cast<uint16_t>(temporaryVramAddress.nametableY);
            vramAddress.coarseY = static_cast<uint16_t>(temporaryVramAddress.coarseY);
        }
    }

    doRenderingPipeline();
}

void PPU::visibleScanlines() {
    doRenderingPipeline();

    if (cycle >= 1 && cycle <= 256) {
        if (cycle == 1) {
            // TODO: Not cycle accruate
            fillCurrentScanlineSprites();
        }

        drawPixel();

        if (scanline == 239 && cycle == 256) {
            // We have finished drawing all visible pixels, so the display is ready
            std::swap(workingDisplay, finishedDisplay);

            frameReadyFlag = true;
        }
    }
    else if (cycle == 280) { // TODO: Think this should really be 260, but breaks things...
        if (isRenderingEnabled()) {
            handleMapper4IRQ();
        }
    }
}

void PPU::verticalBlankScanlines() {
    if (scanline == 241 && cycle == 1) {
        status.vBlankStarted = 1;
        if (control.nmiEnabled) {
            nmiDelayCounter = NMI_DELAY_TIME;
        }
    }
}

void PPU::doRenderingPipeline() {
    if (cycle >= 1 && cycle <= 256) {
        doStandardFetchCycle();

        if (cycle == 256) {
            if (isRenderingEnabled()) {
                incrementY();
            }
        }
    }
    else if (cycle >= 257 && cycle <= 320) {
        if (cycle == 257) {
            if (mask.showBackground) {
                reloadShifters();
            }

            if (isRenderingEnabled()) {
                vramAddress.coarseX = static_cast<uint16_t>(temporaryVramAddress.coarseX);
                vramAddress.nametableX = static_cast<uint16_t>(temporaryVramAddress.nametableX);
            }
        }

        switch (cycle % 8) {
            case 1: case 3: fetchNameTableByte(); break; // Garbage nametable fetches
        }
    }
    else if (cycle >= 321 && cycle <= 336) {
        doStandardFetchCycle();
    }
    else { // if (cycle >= 337 && cycle <= 340) {
        if (cycle == 337 || cycle == 339) fetchNameTableByte(); // Unused nametable fetches
    }
}

void PPU::doStandardFetchCycle() {
    if (mask.showBackground) {
        shiftShifters();
    }

    switch (cycle % 8) {
        case 1:
            if (mask.showBackground) {
                reloadShifters();
            }
            fetchNameTableByte();
            break;
        case 3:
            fetchAttributeTableByte();
            break;
        case 5:
            fetchPatternTableByteLo();
            break;
        case 7:
            fetchPatternTableByteHi();
            break;
        case 0:
            if (isRenderingEnabled()) {
                incrementCoarseX();
            }
            break;
    }
}

void PPU::fetchNameTableByte() {
    nextNameTableByte = readNameTable(0x2000 + (vramAddress.data & 0x0FFF));
}

void PPU::fetchAttributeTableByte() {
    uint16_t offset =
        (vramAddress.nametableY << 11) |
        (vramAddress.nametableX << 10) |
        ((vramAddress.coarseY >> 2) << 3) |
        (vramAddress.coarseX >> 2);
    uint8_t nextAttributeTableByte = readNameTable(0x23C0 + offset);

    // Extract the correct 2 bit portion of the attribute table byte
    if (vramAddress.coarseY & 0x02) {
        nextAttributeTableByte >>= 4;
    }
    if (vramAddress.coarseX & 0x02) {
        nextAttributeTableByte >>= 2;
    }
    nextAttributeTableLo = nextAttributeTableByte & 0x1;
    nextAttributeTableHi = nextAttributeTableByte & 0x2;
}

void PPU::fetchPatternTableByteLo() {
    uint16_t address =
        (control.backgroundPatternTable << 12) |
        (nextNameTableByte << 4) |
        vramAddress.fineY;
    nextPatternTableLo = cartridge.mapper->mapCHRRead(address);
}

void PPU::fetchPatternTableByteHi() {
    uint16_t address =
        (control.backgroundPatternTable << 12) |
        (nextNameTableByte << 4) |
        vramAddress.fineY;
    nextPatternTableHi = cartridge.mapper->mapCHRRead(address + 8);
}

void PPU::drawPixel() {
    // Get color from background
    uint8_t backgroundPatternTable = 0;
    uint8_t backgroundAttributeTable = 0;
    if (mask.showBackground) {
        if (mask.showBackgroundLeft || cycle >= 9) {
            uint8_t shift = 15 - fineX;

            bool backgroundPatternTableLo = (patternTableLoShifter >> shift) & 1;
            bool backgroundPatternTableHi = (patternTableHiShifter >> shift) & 1;
            backgroundPatternTable = (backgroundPatternTableHi << 1) | static_cast<uint8_t>(backgroundPatternTableLo);

            bool backgroundAttributeTableLo = (attributeTableLoShifter >> shift) & 1;
            bool backgroundAttributeTableHi = (attributeTableHiShifter >> shift) & 1;
            backgroundAttributeTable = (backgroundAttributeTableHi << 1) | static_cast<uint8_t>(backgroundAttributeTableLo);
        }
    }
    uint16_t backgroundAddr = getPalleteRamAddress(backgroundPatternTable, backgroundAttributeTable);
    uint8_t backgroundColorIndex = viewPalleteRam(backgroundAddr);

    // Get color from sprites
    uint8_t spritePatternTable = 0;
    uint8_t spriteAttributeTable = 0;
    bool spritePriority = 0;
    bool sprite0Rendered = false;
    if (mask.showSprites) {
        if (mask.showSpritesLeft || cycle >= 9) {
            for (int i = 0; i < static_cast<int>(currentScanlineSprites.size()); i++) {
                const SpriteData& spriteData = currentScanlineSprites[i];
                const OAMEntry& sprite = spriteData.oam;

                int differenceX = (cycle - 1) - sprite.x;
                if (differenceX < 0 || differenceX >= 8) {
                    continue;
                }

                uint8_t x = differenceX;

                bool flipHorizontal = (sprite.attributes >> 6) & 1;
                if (flipHorizontal) {
                    x = 7 - x;
                }

                uint8_t shift = 7 - x;
                bool spritePatternTableBitLo = (spriteData.patternTableLo >> shift) & 1;
                bool spritePatternTableBitHi = (spriteData.patternTableHi >> shift) & 1;
                spritePatternTable = (spritePatternTableBitHi << 1) | static_cast<uint8_t>(spritePatternTableBitLo);

                spriteAttributeTable = 0x4 | (sprite.attributes & 0x3);
                spritePriority = !((sprite.attributes >> 5) & 1);

                // If the sprite pattern table is > 0, then it isn't transparent.
                // In that case we should draw it and ignore the rest of the sprites.
                // This is because priority between sprites is determined by their location in OAM (priority between sprite and background is determined by spritePriority variable)
                if (spritePatternTable) {
                    if (i == 0 && sprite0OnCurrentScanline) {
                        sprite0Rendered = true;
                    }
                    break;
                }
            }
        }
    }
    uint16_t spriteAddr = getPalleteRamAddress(spritePatternTable, spriteAttributeTable);
    uint8_t spriteColorIndex = viewPalleteRam(spriteAddr);

    // Now combine the background color, sprite color, and priority to get the final color
    uint8_t finalColorIndex;

    bool bothTransparent = (backgroundPatternTable == 0 && spritePatternTable == 0);
    // bool onlyBackgroundTransparent = (backgroundPatternTable == 0 && spritePatternTable > 0);
    bool onlySpriteTransparent = (backgroundPatternTable > 0 && spritePatternTable == 0);
    bool bothVisible = (backgroundPatternTable > 0 && spritePatternTable > 0);

    if (bothTransparent || onlySpriteTransparent || (bothVisible && spritePriority == 0)) {
        finalColorIndex = backgroundColorIndex;
    }
    else { // if (onlyBackgroundTransparent || (bothVisible && spritePriority == 1)) {
        finalColorIndex = spriteColorIndex;
    }

    if (sprite0Rendered && bothVisible && mask.showBackground && mask.showSprites && (cycle - 1) != 0xFF) {
        bool renderingLeft = mask.showBackgroundLeft && mask.showSpritesLeft;
        if (renderingLeft || (!renderingLeft && cycle >= 9)) {
            status.sprite0Hit = 1;
        }
    }

    uint32_t finalColor = SCREEN_COLORS[finalColorIndex];

    // Modify the final color based on the PPU's emphasis bits
    if (mask.emphRed || mask.emphGreen || mask.emphBlue) {
        uint8_t colorColumn = finalColorIndex & 0xF;
        if (colorColumn != 0xE && colorColumn != 0xF) {
            BitField<16, 8, uint32_t> red{ finalColor };
            BitField<8, 8, uint32_t>  green{ finalColor };
            BitField<0, 8, uint32_t>  blue{ finalColor };

            uint8_t attenuationRed = 0;
            uint8_t attenuationGreen = 0;
            uint8_t attenuationBlue = 0;

            if (mask.emphRed) {
                attenuationGreen++;
                attenuationBlue++;
            }
            if (mask.emphGreen) {
                attenuationRed++;
                attenuationBlue++;
            }
            if (mask.emphBlue) {
                attenuationRed++;
                attenuationGreen++;
            }

            red = ATTENUATION_TABLE[(attenuationRed << 8) | red];
            green = ATTENUATION_TABLE[(attenuationGreen << 8) | green];
            blue = ATTENUATION_TABLE[(attenuationBlue << 8) | blue];
        }
    }

    (*workingDisplay)[scanline][cycle - 1] = finalColor;
}

void PPU::reloadShifters() {
    auto reloadShifter = [](uint16_t& shiftRegister, uint8_t data) {
        shiftRegister &= 0xFF00;
        shiftRegister |= data;
    };

    reloadShifter(patternTableLoShifter, nextPatternTableLo);
    reloadShifter(patternTableHiShifter, nextPatternTableHi);
    reloadShifter(attributeTableLoShifter, nextAttributeTableLo ? 0xFF : 0x00);
    reloadShifter(attributeTableHiShifter, nextAttributeTableHi ? 0xFF : 0x00);
}

void PPU::shiftShifters() {
    patternTableLoShifter <<= 1;
    patternTableHiShifter <<= 1;
    attributeTableLoShifter <<= 1;
    attributeTableHiShifter <<= 1;
}

bool PPU::isRenderingEnabled() const {
    return mask.showBackground || mask.showSprites;
}

void PPU::incrementCycle() {
    if (cycle < 340) {
        cycle++;
    }
    else {
        if (scanline < 260) {
            scanline++;
        }
        else {
            scanline = -1;
            oddFrame ^= 1;
        }

        // Skip a cycle on odd frame numbers
        if (scanline == 0 && oddFrame) {
            cycle = 1;
        }
        else {
            cycle = 0;
        }
    }
}

// The code for this function is based on pseudocode from https://www.nesdev.org/wiki/PPU_scrolling
void PPU::incrementCoarseX() {
    uint16_t& v = vramAddress.data;

    if ((v & 0x001F) == 31) { // if coarse X == 31
        v &= ~0x001F; // coarse X = 0
        v ^= 0x0400; // switch horizontal nametable
    }
    else {
        v += 1; // increment coarse X
    }
}

// The code for this function is based on pseudocode from https://www.nesdev.org/wiki/PPU_scrolling
void PPU::incrementY() {
    uint16_t& v = vramAddress.data;

    if ((v & 0x7000) != 0x7000) { // if fine Y < 7
        v += 0x1000; // increment fine Y
    }
    else {
        v &= ~0x7000; // fine Y = 0            
        int y = (v & 0x03E0) >> 5; // let y = coarse Y 
        if (y == 29) {
            y = 0; // coarse Y = 0                    
            v ^= 0x0800; // switch vertical nametable
        }
        else if (y == 31) {
            y = 0; // coarse Y = 0, nametable not switched
        }
        else {
            y += 1; // increment coarse Y
        }
        v = (v & ~0x03E0) | (y << 5); // put coarse Y back into v
    }
}

void PPU::fillCurrentScanlineSprites() {
    currentScanlineSprites.clear();
    sprite0OnCurrentScanline = false;
    for (int i = 0; i < OAM_SPRITES; i++) {
        uint8_t index = i << 2;
        OAMEntry sprite = {
            oamBuffer[index],
            oamBuffer[index + 1],
            oamBuffer[index + 2],
            oamBuffer[index + 3]
        };

        uint8_t spriteHeight = control.spriteSize ? 16 : 8;

        // NES sprite renders are delayed by one scanline, so they will end up one scanline below where it is specified in OAM
        // As a result, NES programmers place their sprite value MINUS 1 into OAM.
        // I manually remove this offset by adding 1 because I determine which sprites will be rendered for a particular scanline at the start of a scanline, not during a previous one.
        int differenceY = scanline - (sprite.y + 1);

        if (differenceY >= 0 && differenceY < spriteHeight) {
            if (currentScanlineSprites.size() == MAX_SPRITES) {
                status.spriteOverflow = true;
                break;
            }
            else {
                // Adding 1 to the y value of visible sprites for aforementioned reason
                sprite.y++;

                uint8_t y = scanline - sprite.y;

                bool flipVertical = (sprite.attributes >> 7) & 1;
                if (flipVertical) {
                    y = control.spriteSize ? (15 - y) : (7 - y);
                }

                uint16_t spritePatternTableAddr;

                if (!control.spriteSize) {
                    spritePatternTableAddr =
                        (control.spritePatternTable << 12) |
                        (sprite.tileIndex << 4) |
                        y;
                }
                else {
                    if (y < 8) {
                        spritePatternTableAddr =
                            ((sprite.tileIndex & 0x1) << 12) |
                            ((sprite.tileIndex & 0xFE) << 4) |
                            y;
                    }
                    else {
                        spritePatternTableAddr =
                            ((sprite.tileIndex & 0x1) << 12) |
                            (((sprite.tileIndex & 0xFE) | 1) << 4) |
                            (y & 0x7);
                    }
                }

                uint8_t spritePatternTableLo = cartridge.mapper->mapCHRRead(spritePatternTableAddr);
                uint8_t spritePatternTableHi = cartridge.mapper->mapCHRRead(spritePatternTableAddr + 8);

                currentScanlineSprites.push_back({ sprite, spritePatternTableLo, spritePatternTableHi });

                if (i == 0) {
                    sprite0OnCurrentScanline = true;
                }
            }
        }
    }
}

uint16_t PPU::getPalleteRamAddress(uint8_t patternTable, uint8_t attributeTable) const {
    return PALLETE_RAM_RANGE.lo + (attributeTable << 2) + patternTable;
}

std::array<uint32_t, 0x20> PPU::getPalleteRamColors() const {
    std::array<uint32_t, 0x20> result;
    for (int i = 0; i < 0x20; i++) {
        uint8_t index = getPalleteRamIndexRead(i);
        result[i] = SCREEN_COLORS[palleteRam[index] & 0x3F];
    }
    return result;
}

void PPU::serialize(Serializer& s) const {
    s.serializeUInt8(control.data);
    s.serializeUInt8(mask.data);
    s.serializeUInt8(status.data);
    s.serializeBool(addressLatch);
    s.serializeUInt16(temporaryVramAddress.data);
    s.serializeUInt16(vramAddress.data);
    s.serializeUInt8(fineX);
    s.serializeUInt8(ppuBusData);
    s.serializeArray(palleteRam, s.uInt8Func);
    s.serializeArray(nameTable, s.uInt8Func);
    s.serializeInt32(scanline);
    s.serializeInt32(cycle);
    s.serializeBool(oddFrame);
    s.serializeUInt16(patternTableLoShifter);
    s.serializeUInt16(patternTableHiShifter);
    s.serializeUInt16(attributeTableLoShifter);
    s.serializeUInt16(attributeTableHiShifter);
    s.serializeUInt8(nextNameTableByte);
    s.serializeUInt8(nextPatternTableLo);
    s.serializeUInt8(nextPatternTableHi);
    s.serializeBool(nextAttributeTableLo);
    s.serializeBool(nextAttributeTableHi);
    s.serializeUInt8(oamAddress);
    s.serializeBool(nmiRequest);
    s.serializeBool(irqRequest);
    s.serializeArray(oamBuffer, s.uInt8Func);

    if (s.version.minor >= 1) {
        std::function<void(const SpriteData&)> spriteDataFunc = [&](const SpriteData& spriteData) -> void {
            uint32_t oam =
                spriteData.oam.y |
                (spriteData.oam.tileIndex << 8) |
                (spriteData.oam.attributes << 16) |
                (spriteData.oam.x << 24);
            s.serializeUInt32(oam); // TODO: In future version make each field a seperate entry
            s.serializeUInt8(spriteData.patternTableLo);
            s.serializeUInt8(spriteData.patternTableHi);
        };

        s.serializeVector(currentScanlineSprites, spriteDataFunc);
        s.serializeBool(sprite0OnCurrentScanline);
        s.serializeUInt8(nmiDelayCounter);
    }
}

void PPU::deserialize(Deserializer& d) {
    d.deserializeUInt8(control.data);
    d.deserializeUInt8(mask.data);
    d.deserializeUInt8(status.data);
    d.deserializeBool(addressLatch);
    d.deserializeUInt16(temporaryVramAddress.data);
    d.deserializeUInt16(vramAddress.data);
    d.deserializeUInt8(fineX);
    d.deserializeUInt8(ppuBusData);
    d.deserializeArray(palleteRam, d.uInt8Func);
    d.deserializeArray(nameTable, d.uInt8Func);
    d.deserializeInt32(scanline);
    d.deserializeInt32(cycle);
    d.deserializeBool(oddFrame);
    d.deserializeUInt16(patternTableLoShifter);
    d.deserializeUInt16(patternTableHiShifter);
    d.deserializeUInt16(attributeTableLoShifter);
    d.deserializeUInt16(attributeTableHiShifter);
    d.deserializeUInt8(nextNameTableByte);
    d.deserializeUInt8(nextPatternTableLo);
    d.deserializeUInt8(nextPatternTableHi);
    d.deserializeBool(nextAttributeTableLo);
    d.deserializeBool(nextAttributeTableHi);
    d.deserializeUInt8(oamAddress);
    d.deserializeBool(nmiRequest);
    d.deserializeBool(irqRequest);
    d.deserializeArray(oamBuffer, d.uInt8Func);

    if (d.version.minor >= 1) {
        std::function<void(SpriteData&)> spriteDataFunc = [&](SpriteData& spriteData) -> void {
            uint32_t oamTemp;
            d.deserializeUInt32(oamTemp); // TODO: In future version make each field a seperate entry
            spriteData.oam.y = oamTemp & 0xFF;
            spriteData.oam.tileIndex = (oamTemp >> 8) & 0xFF;
            spriteData.oam.attributes = (oamTemp >> 16) & 0xFF;
            spriteData.oam.x = (oamTemp >> 24) & 0xFF;

            d.deserializeUInt8(spriteData.patternTableLo);
            d.deserializeUInt8(spriteData.patternTableHi);
        };

        d.deserializeVector(currentScanlineSprites, spriteDataFunc);
        d.deserializeBool(sprite0OnCurrentScanline);
        d.deserializeUInt8(nmiDelayCounter);
    }
    else {
        currentScanlineSprites.clear();
        sprite0OnCurrentScanline = false;
        nmiDelayCounter = 0;
    }
}