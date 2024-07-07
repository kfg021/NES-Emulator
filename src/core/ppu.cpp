#include "core/ppu.hpp"

#include "core/mapper/mapper4.hpp"

// Screen colors taken from https://www.nesdev.org/wiki/PPU_palettes
const std::array<uint32_t, PPU::NUM_SCREEN_COLORS> PPU::SCREEN_COLORS = {
    0xFF626262, 0xFF001FB2, 0xFF2404C8, 0xFF5200B2, 0xFF730076, 0xFF800024, 0xFF730B00, 0xFF522800, 0xFF244400, 0xFF005700, 0xFF005C00, 0xFF005324, 0xFF003C76, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFABABAB, 0xFF0D57FF, 0xFF4B30FF, 0xFF8A13FF, 0xFFBC08D6, 0xFFD21269, 0xFFC72E00, 0xFF9D5400, 0xFF607B00, 0xFF209800, 0xFF00A300, 0xFF009942, 0xFF007DB4, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFF53AEFF, 0xFF9085FF, 0xFFD365FF, 0xFFFF57FF, 0xFFFF5DCF, 0xFFFF7757, 0xFFFA9E00, 0xFFBDC700, 0xFF7AE700, 0xFF43F611, 0xFF26EF7E, 0xFF2CD5F6, 0xFF4E4E4E, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFFB6E1FF, 0xFFCED1FF, 0xFFE9C3FF, 0xFFFFBCFF, 0xFFFFBDF4, 0xFFFFC6C3, 0xFFFFD59A, 0xFFE9E681, 0xFFCEF481, 0xFFB6FB9A, 0xFFA9FAC3, 0xFFA9F0F4, 0xFFB8B8B8, 0xFF000000, 0xFF000000,
};

PPU::PPU() {
    workingDisplay = std::make_unique<Display>();
    finishedDisplay = std::make_unique<Display>();
}

void PPU::setBus(Bus* bus) {
    this->bus = bus;
}

void PPU::setCartridge(std::shared_ptr<Cartridge>& cartridge) {
    this->cartridge = cartridge;
}

void PPU::initPPU() {
    control = 0;
    mask = 0;
    status = 0;

    ppuBusData = 0;
    vramAddress = 0;
    temporaryVramAddress = 0;
    addressLatch = 0;
    palleteRam = {};

    scanline = 0;
    cycle = 0;
    frame = 0;

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

    (*workingDisplay) = {};
    (*finishedDisplay) = {};

    // Reset the OAM buffer to 0xFF so that sprites start off the screen
    std::fill(oamBuffer.begin(), oamBuffer.end(), 0xFF);

    oamAddress = 0;

    currentScanlineSprites = {};
    sprite0OnCurrentScanline = false;
}

uint8_t PPU::view(uint8_t ppuRegister) const {
    if (ppuRegister == static_cast<int>(Register::PPUSTATUS)) {
        return status.toUInt8();
    }
    else if (ppuRegister == static_cast<int>(Register::OAMDATA)) {
        return oamBuffer[oamAddress];
    }
    else if (ppuRegister == static_cast<int>(Register::PPUDATA)) {
        uint8_t data = ppuBusData;

        // Pallete addresses get returned immediately
        if (PALLETE_RAM_RANGE.contains(vramAddress.toUInt16() & 0x3FFF)) {
            data = ppuView(vramAddress.toUInt16() & 0x3FFF);
        }
        return data;
    }
    else {
        return 0;
    }
}

uint8_t PPU::read(uint8_t ppuRegister) {
    if (ppuRegister == static_cast<int>(Register::PPUSTATUS)) {
        uint8_t data = status.toUInt8();
        status.vBlankStarted = 0;
        addressLatch = 0;
        return data;
    }
    else if (ppuRegister == static_cast<int>(Register::OAMDATA)) {
        return oamBuffer[oamAddress];
    }
    else if (ppuRegister == static_cast<int>(Register::PPUDATA)) {
        uint8_t data = ppuBusData;

        ppuBusData = ppuRead(vramAddress.toUInt16() & 0x3FFF);

        // open bus is the bottom 5 bits of the bus
        status.openBus = ppuBusData & 0x1F;

        // pallete addresses get returned immediately
        if (PALLETE_RAM_RANGE.contains(vramAddress.toUInt16() & 0x3FFF)) {
            data = ppuBusData;
        }

        vramAddress = vramAddress.toUInt16() + (control.vramAddressIncrement ? 32 : 1);

        return data;
    }
    else {
        return 0;
    }
}

void PPU::write(uint8_t ppuRegister, uint8_t value) {
    if (ppuRegister == static_cast<int>(Register::PPUCTRL)) {
        bool oldNmiFlag = control.nmiEnabled;

        control.setFromUInt8(value);

        bool newNmiFlag = control.nmiEnabled;

        // From (https://www.nesdev.org/wiki/PPU_registers#PPUCTRL): 
        // If the PPU is currently in vertical blank, and the PPUSTATUS ($2002) vblank flag is still set (1), changing the NMI flag in bit 7 of $2000 from 0 to 1 will immediately generate an NMI. 
        if (status.vBlankStarted && !oldNmiFlag && newNmiFlag) {
            bus->nmiRequest = true;
        }

        temporaryVramAddress.nametableX = control.nametableX;
        temporaryVramAddress.nametableY = control.nametableY;
    }
    else if (ppuRegister == static_cast<int>(Register::PPUMASK)) {
        mask.setFromUInt8(value);
    }
    else if (ppuRegister == static_cast<int>(Register::OAMADDR)) {
        oamAddress = value;
    }
    else if (ppuRegister == static_cast<int>(Register::OAMDATA)) {
        oamBuffer[oamAddress] = value;
    }
    else if (ppuRegister == static_cast<int>(Register::PPUSCROLL)) {
        if (addressLatch == 0) {
            fineX = value & 0x7;
            temporaryVramAddress.coarseX = value >> 3;
        }
        else {
            temporaryVramAddress.fineY = value & 0x7;
            temporaryVramAddress.coarseY = value >> 3;
        }
        addressLatch ^= 1;
    }
    else if (ppuRegister == static_cast<int>(Register::PPUADDR)) {
        if (addressLatch == 0) {
            temporaryVramAddress = temporaryVramAddress.toUInt16() & 0x00FF;
            temporaryVramAddress = temporaryVramAddress.toUInt16() | (value << 8);
        }
        else {
            temporaryVramAddress = temporaryVramAddress.toUInt16() & 0xFF00;
            temporaryVramAddress = temporaryVramAddress.toUInt16() | value;

            vramAddress = temporaryVramAddress;
        }
        addressLatch ^= 1;
    }
    else if (ppuRegister == static_cast<int>(Register::PPUDATA)) {
        ppuWrite(vramAddress.toUInt16() & 0x3FFF, value);

        vramAddress = vramAddress.toUInt16() + (control.vramAddressIncrement ? 32 : 1);
    }
}

uint8_t PPU::getPalleteRamIndex(uint16_t address, bool read) const {
    address &= 0x1F;

    // Address mirroring to handle background tiles
    // When we are reading from the pallete ram, we consider every 4th tile to be equivalent to the background pixel
    // But, it turns out that these pallete ram indices can actually be set (setting them just has no effect)
    if (read) {
        if ((address & 0x3) == 0) {
            address = 0;
        }
    }
    else {
        if (address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1C) {
            address &= 0x0F;
        }
    }

    return address;
}

uint16_t PPU::getNameTableIndex(uint16_t address) const {
    // Nametable address is 12 bits
    address &= 0xFFF;

    static constexpr MemoryRange NAMETABLE_QUAD_1{ 0x000, 0x3FF };
    static constexpr MemoryRange NAMETABLE_QUAD_2{ 0x400, 0x7FF };
    static constexpr MemoryRange NAMETABLE_QUAD_3{ 0x800, 0xBFF };
    // static constexpr MemoryRange NAMETABLE_QUAD_4{ 0xC00, 0xFFF };

    // Bits 10 and 11 determine the nametable, and we can modify them to map any address onto a specific nametable.
    // Setting {bit10, bit11} = {0, 0} maps to nametable A
    // Setting {bit10, bit11} = {0, 1} maps to nametable B
    auto mapToNameTableA = [](uint16_t address) {
        return address & 0x3FF;
    };
    auto mapToNameTableB = [](uint16_t address) {
        return (address & 0x3FF) | 0x400;
    };

    Mapper::MirrorMode mirrorMode = cartridge->getMirrorMode();
    if (mirrorMode == Mapper::MirrorMode::HORIZONTAL) {
        if (NAMETABLE_QUAD_1.contains(address) || NAMETABLE_QUAD_2.contains(address)) {
            return mapToNameTableA(address);
        }
        else { // if (NAMETABLE_QUAD_3.contains(address) || NAMETABLE_QUAD_4.contains(address)) {
            return mapToNameTableB(address);
        }
    }
    else if (mirrorMode == Mapper::MirrorMode::VERTICAL) {
        if (NAMETABLE_QUAD_1.contains(address) || NAMETABLE_QUAD_3.contains(address)) {
            return mapToNameTableA(address);
        }
        else { // if (NAMETABLE_QUAD_2.contains(address) || NAMETABLE_QUAD_4.contains(address)) {
            return mapToNameTableB(address);
        }
    }
    else if (mirrorMode == Mapper::MirrorMode::ONE_SCREEN_LOWER_BANK) {
        return mapToNameTableA(address);
    }
    else { // if (mirrorMode == Mapper::MirrorMode::ONE_SCREEN_UPPER_BANK) {
        return mapToNameTableB(address);
    }
}

uint8_t PPU::ppuView(uint16_t address) const {
    if (PATTERN_TABLE_RANGE.contains(address)) {
        return cartridge->mapper->mapCHRView(address);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        if (cartridge->getMirrorMode() != Mapper::MirrorMode::FOUR_SCREEN) {
            return nameTable[getNameTableIndex(address)];
        }
        else {
            // Mapper handles nametables in 4 screen mode
            return cartridge->mapper->mapCHRView(address);
        }
    }
    else if (PALLETE_RAM_RANGE.contains(address)) {
        uint8_t data = palleteRam[getPalleteRamIndex(address, true)] & 0x3F;
        if (mask.greyscale) {
            data &= 0x30;
        }
        return data;
    }
    else {
        return 0;
    }
}

uint8_t PPU::ppuRead(uint16_t address) {
    if (PATTERN_TABLE_RANGE.contains(address)) {
        return cartridge->mapper->mapCHRRead(address);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        if (cartridge->getMirrorMode() != Mapper::MirrorMode::FOUR_SCREEN) {
            return nameTable[getNameTableIndex(address)];
        }
        else {
            // Mapper handles nametables in 4 screen mode
            return cartridge->mapper->mapCHRRead(address);
        }
    }
    else if (PALLETE_RAM_RANGE.contains(address)) {
        uint8_t data = palleteRam[getPalleteRamIndex(address, true)] & 0x3F;
        if (mask.greyscale) {
            data &= 0x30;
        }
        return data;
    }
    else {
        return 0;
    }
}

void PPU::ppuWrite(uint16_t address, uint8_t value) {
    if (PATTERN_TABLE_RANGE.contains(address)) {
        cartridge->mapper->mapCHRWrite(address, value);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        if (cartridge->getMirrorMode() != Mapper::MirrorMode::FOUR_SCREEN) {
            nameTable[getNameTableIndex(address)] = value;
        }
        else {
            // Mapper handles nametables in 4 screen mode
            cartridge->mapper->mapCHRWrite(address, value);
        }
    }
    else if (PALLETE_RAM_RANGE.contains(address)) {
        palleteRam[getPalleteRamIndex(address, false)] = value;
    }
}

PPU::PatternTable PPU::getPatternTable(bool isBackground, uint8_t palleteNumber) const {
    PatternTable table{};

    bool tableNumber = isBackground ? control.backgroundPatternTable : control.spritePatternTable;

    for (int tileRow = 0; tileRow < PATTERN_TABLE_NUM_TILES; tileRow++) {
        for (int tileCol = 0; tileCol < PATTERN_TABLE_NUM_TILES; tileCol++) {
            uint16_t tableOffset = PATTERN_TABLE_TILE_BYTES * (PATTERN_TABLE_NUM_TILES * tileRow + tileCol);

            for (int spriteRow = 0; spriteRow < PATTERN_TABLE_TILE_SIZE; spriteRow++) {
                uint8_t loBits = ppuView(PATTERN_TABLE_TOTAL_BYTES * tableNumber + tableOffset + spriteRow);
                uint8_t hiBits = ppuView(PATTERN_TABLE_TOTAL_BYTES * tableNumber + tableOffset + spriteRow + 0x8);

                for (int spriteCol = 0; spriteCol < PATTERN_TABLE_TILE_SIZE; spriteCol++) {
                    bool currentLoBit = (loBits >> spriteCol) & 1;
                    bool currentHiBit = (hiBits >> spriteCol) & 1;
                    uint8_t palleteIndex = ((!isBackground) << 4) | (currentHiBit << 1) | static_cast<uint8_t>(currentLoBit);

                    uint16_t pixelRow = PATTERN_TABLE_TILE_SIZE * tileRow + spriteRow;
                    uint16_t pixelCol = PATTERN_TABLE_TILE_SIZE * tileCol + PATTERN_TABLE_TILE_SIZE - 1 - spriteCol;

                    uint16_t addr = getPalleteRamAddress(palleteIndex, palleteNumber);
                    table[pixelRow][pixelCol] = SCREEN_COLORS[ppuView(addr) & 0x3F];
                }
            }
        }
    }
    return table;
}

void PPU::executeCycle() {
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
    if (cartridge->mapper->config.id == 4) {
        // We can static_cast instead of dynamic_cast because we explicitly checked id
        Mapper4* mapper4 = static_cast<Mapper4*>(cartridge->mapper.get());
        if (mapper4->irqRequestAtEndOfScanline()) {
            bus->irqRequest = true;
        }
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
            vramAddress.fineY = temporaryVramAddress.fineY;
            vramAddress.nametableY = temporaryVramAddress.nametableY;
            vramAddress.coarseY = temporaryVramAddress.coarseY;
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
        }
    }
    else if (cycle == 260) {
        if (isRenderingEnabled()) {
            handleMapper4IRQ();
        }
    }
}

void PPU::verticalBlankScanlines() {
    if (scanline == 241 && cycle == 1) {
        status.vBlankStarted = 1;
        if (control.nmiEnabled) {
            bus->nmiRequest = true;
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
                vramAddress.coarseX = temporaryVramAddress.coarseX;
                vramAddress.nametableX = temporaryVramAddress.nametableX;
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
    nextNameTableByte = ppuRead(0x2000 + (vramAddress.toUInt16() & 0x0FFF));
}

void PPU::fetchAttributeTableByte() {
    uint16_t offset =
        (vramAddress.nametableY << 11) |
        (vramAddress.nametableX << 10) |
        ((vramAddress.coarseY >> 2) << 3) |
        (vramAddress.coarseX >> 2);
    uint8_t nextAttributeTableByte = ppuRead(0x23C0 + offset);

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
    nextPatternTableLo = ppuRead(address);
}

void PPU::fetchPatternTableByteHi() {
    uint16_t address =
        (control.backgroundPatternTable << 12) |
        (nextNameTableByte << 4) |
        vramAddress.fineY;
    nextPatternTableHi = ppuRead(address + 8);
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
    uint32_t backgroundColor = SCREEN_COLORS[ppuRead(backgroundAddr) & 0x3F];

    // Get color from sprites
    uint8_t spritePatternTable = 0;
    uint8_t spriteAttributeTable = 0;
    bool spritePriority = 0;
    bool sprite0Rendered = false;
    if (mask.showSprites) {
        if (mask.showSpritesLeft || cycle >= 9) {
            for (int i = 0; i < static_cast<int>(currentScanlineSprites.size()); i++) {
                const OAMEntry& sprite = currentScanlineSprites[i];
                int differenceX = (cycle - 1) - sprite.x;
                if (differenceX < 0 || differenceX >= 8) {
                    continue;
                }

                uint8_t x = differenceX;
                uint8_t y = scanline - sprite.y;

                bool flipHorizontal = (sprite.attributes >> 6) & 1;
                if (flipHorizontal) {
                    x = 7 - x;
                }

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

                uint8_t spritePatternTableLo = ppuRead(spritePatternTableAddr);
                uint8_t spritePatternTableHi = ppuRead(spritePatternTableAddr + 8);

                uint8_t shift = 7 - x;
                bool spritePatternTableBitLo = (spritePatternTableLo >> shift) & 1;
                bool spritePatternTableBitHi = (spritePatternTableHi >> shift) & 1;
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
    uint32_t spriteColor = SCREEN_COLORS[ppuRead(spriteAddr) & 0x3F];

    // Now combine the background color, sprite color, and priority to get the final color
    uint32_t finalColor;

    bool bothTransparent = (backgroundPatternTable == 0 && spritePatternTable == 0);
    // bool onlyBackgroundTransparent = (backgroundPatternTable == 0 && spritePatternTable > 0);
    bool onlySpriteTransparent = (backgroundPatternTable > 0 && spritePatternTable == 0);
    bool bothVisible = (backgroundPatternTable > 0 && spritePatternTable > 0);

    if (bothTransparent || onlySpriteTransparent || (bothVisible && spritePriority == 0)) {
        finalColor = backgroundColor;
    }
    else { // if (onlyBackgroundTransparent || (bothVisible && spritePriority == 1)) {
        finalColor = spriteColor;
    }

    if (sprite0Rendered && bothVisible && mask.showBackground && mask.showSprites && (cycle - 1) != 0xFF) {
        bool renderingLeft = mask.showBackgroundLeft && mask.showSpritesLeft;
        if (renderingLeft || (!renderingLeft && cycle >= 9)) {
            status.sprite0Hit = 1;
        }
    }

    // TODO: use the PPU's emphasis bits to modify the final color
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

bool PPU::isRenderingEnabled() {
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
            frame++;
        }

        // Skip a cycle on odd frame numbers
        if (scanline == 0 && (frame & 1)) {
            cycle = 1;
        }
        else {
            cycle = 0;
        }
    }
}

// The code for this function is based on pseudocode from https://www.nesdev.org/wiki/PPU_scrolling
void PPU::incrementCoarseX() {
    uint16_t v = vramAddress.toUInt16();

    if ((v & 0x001F) == 31) { // if coarse X == 31
        v &= ~0x001F; // coarse X = 0
        v ^= 0x0400; // switch horizontal nametable
    }
    else {
        v += 1; // increment coarse X
    }

    vramAddress.setFromUInt16(v);
}

// The code for this function is based on pseudocode from https://www.nesdev.org/wiki/PPU_scrolling
void PPU::incrementY() {
    uint16_t v = vramAddress.toUInt16();

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

    vramAddress.setFromUInt16(v);
}

void PPU::fillCurrentScanlineSprites() {
    static constexpr uint8_t MAX_SPRITES = 8;
    currentScanlineSprites.clear();
    sprite0OnCurrentScanline = false;
    for (int i = 0; i < OAM_SPRITES; i++) {
        OAMEntry sprite(oamBuffer, i);
        uint8_t spriteHeight = control.spriteSize ? 16 : 8;

        // NES sprite renders are delayed by one scanline, so they will end up one scanline below where it is specified in OAM
        // As a result, NES programmers place their sprite value MINUS 1 into OAM.
        // I manually remove this offset by adding because I determine which sprites will be rendered for a particular scanline at the start of a scanline, not during a previous one.
        int differenceY = scanline - (sprite.y + 1);

        if (differenceY >= 0 && differenceY < spriteHeight) {
            if (currentScanlineSprites.size() == MAX_SPRITES) {
                status.spriteOverflow = true;
                break;
            }
            else {
                currentScanlineSprites.push_back(sprite);

                // Adding 1 to the y value of visible sprites for aforementioned reason
                currentScanlineSprites.back().y++;

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
        uint8_t index = getPalleteRamIndex(i, true);
        result[i] = SCREEN_COLORS[palleteRam[index] & 0x3F];
    }
    return result;
}

// Helper struct function definitions
PPU::Control::Control(uint8_t data) {
    setFromUInt8(data);
}

uint8_t PPU::Control::toUInt8() const {
    uint8_t data =
        static_cast<uint8_t>(nametableX) |
        (nametableY << 1) |
        (vramAddressIncrement << 2) |
        (spritePatternTable << 3) |
        (backgroundPatternTable << 4) |
        (spriteSize << 5) |
        (ppuSelect << 6) |
        (nmiEnabled << 7);

    return data;
}

void PPU::Control::setFromUInt8(uint8_t data) {
    nametableX = data & 0x1;
    nametableY = (data >> 1) & 0x1;
    vramAddressIncrement = (data >> 2) & 0x1;
    spritePatternTable = (data >> 3) & 0x1;
    backgroundPatternTable = (data >> 4) & 0x1;
    spriteSize = (data >> 5) & 0x1;
    ppuSelect = (data >> 6) & 0x1;
    nmiEnabled = (data >> 7) & 0x1;
}


PPU::Mask::Mask(uint8_t value) {
    setFromUInt8(value);
}

uint8_t PPU::Mask::toUInt8() const {
    uint8_t data =
        static_cast<uint8_t>(greyscale) |
        (showBackgroundLeft << 1) |
        (showSpritesLeft << 2) |
        (showBackground << 3) |
        (showSprites << 4) |
        (emphRed << 5) |
        (emphGreen << 6) |
        (emphBlue << 7);
    return data;
}

void PPU::Mask::setFromUInt8(uint8_t data) {
    greyscale = data & 0x1;
    showBackgroundLeft = (data >> 1) & 0x1;
    showSpritesLeft = (data >> 2) & 0x1;
    showBackground = (data >> 3) & 0x1;
    showSprites = (data >> 4) & 0x1;
    emphRed = (data >> 5) & 0x1;
    emphGreen = (data >> 6) & 0x1;
    emphBlue = (data >> 7) & 0x1;
}


PPU::Status::Status(uint8_t data) {
    setFromUInt8(data);
}

uint8_t PPU::Status::toUInt8() const {
    uint8_t data =
        openBus |
        (spriteOverflow << 5) |
        (sprite0Hit << 6) |
        (vBlankStarted << 7);
    return data;
}

void PPU::Status::setFromUInt8(uint8_t data) {
    openBus = data & 0x1F;
    spriteOverflow = (data >> 5) & 0x1;
    sprite0Hit = (data >> 6) & 0x1;
    vBlankStarted = (data >> 7) & 0x1;
}

PPU::InternalRegister::InternalRegister(uint16_t data) {
    setFromUInt16(data);
}

uint16_t PPU::InternalRegister::toUInt16() const {
    uint16_t data =
        coarseX |
        (coarseY << 5) |
        (nametableX << 10) |
        (nametableY << 11) |
        (fineY << 12) |
        (unused << 15);
    return data;
}

void PPU::InternalRegister::setFromUInt16(uint16_t data) {
    coarseX = data & 0x1F;
    coarseY = (data >> 5) & 0x1F;
    nametableX = (data >> 10) & 0x1;
    nametableY = (data >> 11) & 0x1;
    fineY = (data >> 12) & 0x7;
    unused = (data >> 15) & 0x1;
}

PPU::OAMEntry::OAMEntry(uint32_t data) {
    setFromUInt32(data);
}

PPU::OAMEntry::OAMEntry(const OAMBuffer& oamBuffer, uint8_t spriteNum) : OAMEntry(0) {
    if (spriteNum < OAM_SPRITES) {
        uint8_t index = spriteNum << 2;
        y = oamBuffer[index];
        tileIndex = oamBuffer[index + 1];
        attributes = oamBuffer[index + 2];
        x = oamBuffer[index + 3];
    }
}

uint32_t PPU::OAMEntry::toUInt32() const {
    uint32_t data =
        y |
        (tileIndex << 8) |
        (attributes << 16) |
        (x << 24);
    return data;
}

void PPU::OAMEntry::setFromUInt32(uint32_t data) {
    y = data & 0xFF;
    tileIndex = (data >> 8) & 0xFF;
    attributes = (data >> 16) & 0xFF;
    x = (data >> 24) & 0xFF;
}