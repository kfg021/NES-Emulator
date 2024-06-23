#include "ppu.hpp"

const std::array<uint32_t, PPU::NUM_SCREEN_COLORS> PPU::SCREEN_COLORS = initScreenColors();

// Screen colors taken from https://www.nesdev.org/wiki/PPU_palettes
std::array<uint32_t, PPU::NUM_SCREEN_COLORS> PPU::initScreenColors() {
    // Colors are 32 bit integers with 8 bits each for a = 0xFF, r, g, b
    std::array<uint32_t, NUM_SCREEN_COLORS> screenColors = {
        0xFF626262, 0xFF001FB2, 0xFF2404C8, 0xFF5200B2, 0xFF730076, 0xFF800024, 0xFF730B00, 0xFF522800, 0xFF244400, 0xFF005700, 0xFF005C00, 0xFF005324, 0xFF003C76, 0xFF000000, 0xFF000000, 0xFF000000,
        0xFFABABAB, 0xFF0D57FF, 0xFF4B30FF, 0xFF8A13FF, 0xFFBC08D6, 0xFFD21269, 0xFFC72E00, 0xFF9D5400, 0xFF607B00, 0xFF209800, 0xFF00A300, 0xFF009942, 0xFF007DB4, 0xFF000000, 0xFF000000, 0xFF000000,
        0xFFFFFFFF, 0xFF53AEFF, 0xFF9085FF, 0xFFD365FF, 0xFFFF57FF, 0xFFFF5DCF, 0xFFFF7757, 0xFFFA9E00, 0xFFBDC700, 0xFF7AE700, 0xFF43F611, 0xFF26EF7E, 0xFF2CD5F6, 0xFF4E4E4E, 0xFF000000, 0xFF000000,
        0xFFFFFFFF, 0xFFB6E1FF, 0xFFCED1FF, 0xFFE9C3FF, 0xFFFFBCFF, 0xFFFFBDF4, 0xFFFFC6C3, 0xFFFFD59A, 0xFFE9E681, 0xFFCEF481, 0xFFB6FB9A, 0xFFA9FAC3, 0xFFA9F0F4, 0xFFB8B8B8, 0xFF000000, 0xFF000000,
    };
    return screenColors;
}

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

    currentPatternTableTileLo = 0;
    currentPatternTableTileHi = 0;
    currentAttributeTableLo = 0;
    currentAttributeTableHi = 0;
    nextNameTableByte = 0;
    nextPatternTableTileLo = 0;
    nextPatternTableTileHi = 0;
    nextAttributeTableLo = 0;
    nextAttributeTableHi = 0;
    fineX = 0;

    nameTable = {};

    (*workingDisplay) = {};
    (*finishedDisplay) = {};

    oamBuffer = {};
    oamAddress = 0;

    currentScanlineSprites = {};
    sprite0OnCurrentScanline = false;
}

uint8_t PPU::view(uint8_t ppuRegister) const {
    if (ppuRegister == (int)Register::PPUSTATUS) {
        return status.toUInt8();
    }
    else if (ppuRegister == (int)Register::OAMDATA) {
        return oamBuffer[oamAddress];
    }
    else if (ppuRegister == (int)Register::PPUDATA) {
        uint8_t data = ppuBusData;

        // pallete addresses get returned immediately
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
    if (ppuRegister == (int)Register::PPUSTATUS) {
        uint8_t data = status.toUInt8();
        status.vBlankStarted = 0;
        addressLatch = 0;
        return data;
    }
    else if (ppuRegister == (int)Register::OAMDATA) {
        return oamBuffer[oamAddress];
    }
    else if (ppuRegister == (int)Register::PPUDATA) {
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
    if (ppuRegister == (int)Register::PPUCTRL) {
        control.setFromUInt8(value);
        temporaryVramAddress.nametableX = control.nametableX;
        temporaryVramAddress.nametableY = control.nametableY;
    }
    else if (ppuRegister == (int)Register::PPUMASK) {
        mask.setFromUInt8(value);
    }
    else if (ppuRegister == (int)Register::OAMADDR) {
        oamAddress = value;
    }
    else if (ppuRegister == (int)Register::OAMDATA) {
        oamBuffer[oamAddress] = value;
    }
    else if (ppuRegister == (int)Register::PPUSCROLL) {
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
    else if (ppuRegister == (int)Register::PPUADDR) {
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
    else if (ppuRegister == (int)Register::PPUDATA) {
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
    // static constexpr MemoryRange NAMETABLE_QUAD_4{0xC00, 0xFFF};

    if (cartridge->getMirrorMode() == Cartridge::MirrorMode::HORIZONTAL) {
        if (NAMETABLE_QUAD_1.contains(address)) {
            return address;
        }
        else if (NAMETABLE_QUAD_2.contains(address)) {
            return address - 0x400;
        }
        else if (NAMETABLE_QUAD_3.contains(address)) {
            return address - 0x400;
        }
        else { // if(NAMETABLE_QUAD_4.contains(address))
            return address - 0x800;
        }
    }
    else if (cartridge->getMirrorMode() == Cartridge::MirrorMode::VERTICAL) {
        if (NAMETABLE_QUAD_1.contains(address) || NAMETABLE_QUAD_2.contains(address)) {
            return address;
        }
        else { // if(NAMETABLE_QUAD_3.contains(address) || NAMETABLE_QUAD_4.contains(address))
            return address - 0x800;
        }
    }
    else {
        return 0;
    }
}

uint8_t PPU::ppuView(uint16_t address) const {
    if (PATTERN_TABLE_RANGE.contains(address)) {
        std::optional<uint8_t> data = cartridge->viewCHR(address);
        return data.value_or(0);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        return nameTable[getNameTableIndex(address)];
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
        std::optional<uint8_t> data = cartridge->readFromCHR(address);
        return data.value_or(0);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        return nameTable[getNameTableIndex(address)];
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
        cartridge->writeToCHR(address, value);
    }
    else if (NAMETABLE_RANGE.contains(address)) {
        nameTable[getNameTableIndex(address)] = value;
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
                    uint8_t palleteIndex = ((!isBackground) << 4) | (currentHiBit << 1) | currentLoBit;

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

// PPU Rendering overview (descriptions from https://www.nesdev.org/wiki/PPU_rendering, see https://www.nesdev.org/w/images/default/4/4f/Ppu.svg for diagram)
// Line-by-line timing
// The PPU renders 262 scanlines per frame. Each scanline lasts for 341 PPU clock cycles (113.667 CPU clock cycles; 1 CPU cycle = 3 PPU cycles), with each clock cycle producing one pixel. The line numbers given here correspond to how the internal PPU frame counters count lines.
// The information in this section is summarized in the diagram in the next section.
// The timing below is for NTSC PPUs. PPUs for 50 Hz TV systems differ:
// Dendy PPUs render 51 post-render scanlines instead of 1
// PAL NES PPUs render 70 vblank scanlines instead of 20, and they additionally run 3.2 PPU cycles per CPU cycle, or 106.5625 CPU clock cycles per scanline.
// TODO: Break this up into functions, fix messy if logic
void PPU::executeCycle() {
    if (scanline >= -1 && scanline < 240) {
        // Pre-render scanline (-1 or 261)
        // This is a dummy scanline, whose sole purpose is to fill the shift registers with the data for the first two tiles of the next scanline. Although no pixels are rendered for this scanline, the PPU still makes the same memory accesses it would for a regular scanline, using whatever the current value of the PPU's V register is, and for the sprite fetches, whatever data is currently in secondary OAM (e.g., the results from scanline 239's sprite evaluation from the previous frame).
        // This scanline varies in length, depending on whether an even or an odd frame is being rendered. For odd frames, the cycle at the end of the scanline is skipped (this is done internally by jumping directly from (339,261) to (0,0), replacing the idle tick at the beginning of the first visible scanline with the last tick of the last dummy nametable fetch). For even frames, the last cycle occurs normally. This is done to compensate for some shortcomings with the way the PPU physically outputs its video signal, the end result being a crisper image when the screen isn't scrolling. However, this behavior can be bypassed by keeping rendering disabled until after this scanline has passed, which results in an image with a "dot crawl" effect similar to, but not exactly like, what's seen in interlaced video.
        // During pixels 280 through 304 of this scanline, the vertical scroll bits are reloaded if rendering is enabled.
        if (scanline == -1) {
            if (cycle == 1) {
                status.vBlankStarted = 0;
                status.sprite0Hit = 0;
                status.spriteOverflow = 0;
            }

            if (cycle >= 280 && cycle <= 304) {
                if (mask.showBackground || mask.showSprites) {
                    vramAddress.fineY = temporaryVramAddress.fineY;
                    vramAddress.nametableY = temporaryVramAddress.nametableY;
                    vramAddress.coarseY = temporaryVramAddress.coarseY;
                }
            }
        }

        if ((cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 336)) { // if((cycle >= 1 && cycle <= 257) || (cycle >= 321 && cycle <= 336)){
            if (mask.showBackground) {
                currentPatternTableTileLo <<= 1;
                currentPatternTableTileHi <<= 1;

                currentAttributeTableLo <<= 1;
                currentAttributeTableHi <<= 1;
            }

            int cycleMod = (cycle - 1) % 8;
            if (cycleMod == 0) {
                updatePatternTable();
                updateAttributeTable();

                nextNameTableByte = ppuRead(0x2000 + (vramAddress.toUInt16() & 0x0FFF));
            }
            else if (cycleMod == 2) {
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
            else if (cycleMod == 4) {
                uint16_t address =
                    (control.backgroundPatternTable << 12) |
                    (nextNameTableByte << 4) |
                    vramAddress.fineY;
                nextPatternTableTileLo = ppuRead(address);
            }
            else if (cycleMod == 6) {
                uint16_t address =
                    (control.backgroundPatternTable << 12) |
                    (nextNameTableByte << 4) |
                    vramAddress.fineY;
                nextPatternTableTileHi = ppuRead(address + 8);
            }
            else if (cycleMod == 7) {
                if (mask.showBackground || mask.showSprites) {
                    incrementCoarseX();
                }
            }
        }

        if (cycle == 256) {
            if (mask.showBackground || mask.showSprites) {
                incrementY();
            }
        }

        if (cycle == 257) {
            updatePatternTable();
            updateAttributeTable();

            if (mask.showBackground || mask.showSprites) {
                vramAddress.nametableX = temporaryVramAddress.nametableX;
                vramAddress.coarseX = temporaryVramAddress.coarseX;
            }
        }

        if (cycle == 337 || cycle == 339) {
            nextNameTableByte = ppuRead(0x2000 + (vramAddress.toUInt16() & 0x0FFF));
        }
    }

    if (scanline == 240) {
    }

    if (scanline == 241) {
        if (cycle == 1) {
            status.vBlankStarted = 1;
            workingDisplay.swap(finishedDisplay);
            if (control.nmiEnabled) {
                bus->nmiRequest = true;
            }
        }
    }

    // TODO: Fix potential off by one errors with cycle / cycle-1
    if (scanline >= 0 && scanline < 240 && (cycle - 1) >= 0 && (cycle - 1) < 256) {
        // Render the current pixel if it is within the bounds of the screen

        // Before we draw anything, get the sprites that are visible on this scanline
        if (cycle == 1) {
            // This implementation is not at all cycle accurate
            // TODO: Consider rendering sprites the way that the NES does
            fillCurrentScanlineSprites();
        }

        // Get color from background
        uint8_t backgroundPatternTable = 0;
        uint8_t backgroundAttributeTable = 0;
        if (mask.showBackground) {
            if (mask.showBackgroundLeft || cycle >= 9) {
                uint8_t shift = 15 - fineX;

                bool backgroundPatternTableLo = (currentPatternTableTileLo >> shift) & 1;
                bool backgroundPatternTableHi = (currentPatternTableTileHi >> shift) & 1;
                backgroundPatternTable = (backgroundPatternTableHi << 1) | backgroundPatternTableLo;

                bool backgroundAttributeTableLo = (currentAttributeTableLo >> shift) & 1;
                bool backgroundAttributeTableHi = (currentAttributeTableHi >> shift) & 1;
                backgroundAttributeTable = (backgroundAttributeTableHi << 1) | backgroundAttributeTableLo;
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
                for (int i = 0; i < (int)currentScanlineSprites.size(); i++) {
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
                    spritePatternTable = (spritePatternTableBitHi << 1) | spritePatternTableBitLo;

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
        else { // if(onlyBackgroundTransparent || (bothVisible && spritePriority == 1))
            finalColor = spriteColor;
        }

        (*workingDisplay)[scanline][cycle - 1] = finalColor;

        if (sprite0Rendered && bothVisible && mask.showBackground && mask.showSprites && (cycle - 1) != 0xFF) {
            bool renderingLeft = mask.showBackgroundLeft && mask.showSpritesLeft;
            if (renderingLeft || (!renderingLeft && cycle >= 9)) {
                status.sprite0Hit = 1;
            }
        }
    }

    // Skip a cycle for odd frame
    // TODO: This might not be working as intended
    if (mask.showBackground || mask.showSprites) {
        if (scanline == -1 && cycle == 339 && !(frame & 1)) {
            cycle++;
        }
    }

    // Advance the cycle/scanline
    cycle++;
    if (cycle > 340) {
        cycle = 0;
        scanline++;
        if (scanline > 260) {
            scanline = -1;
        }
        else if (scanline == 0) {
            frame++;

            OAMEntry entry(oamBuffer, 0);
        }
    }
}

void PPU::updatePatternTable() {
    currentPatternTableTileLo &= 0xFF00;
    currentPatternTableTileLo |= nextPatternTableTileLo;

    currentPatternTableTileHi &= 0xFF00;
    currentPatternTableTileHi |= nextPatternTableTileHi;
}

void PPU::updateAttributeTable() {
    currentAttributeTableLo &= 0xFF00;
    if (nextAttributeTableLo) {
        currentAttributeTableLo |= 0xFF;
    }

    currentAttributeTableHi &= 0xFF00;
    if (nextAttributeTableHi) {
        currentAttributeTableHi |= 0xFF;
    }
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

// The code for these functions is based on pseudocode from https://www.nesdev.org/wiki/PPU_scrolling
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
        nametableX |
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