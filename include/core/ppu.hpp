#ifndef PPU_HPP
#define PPU_HPP

#include "core/cartridge.hpp"
#include "util/serializer.hpp"
#include "util/util.hpp"

#include <memory>
#include <vector>

class PPU {
public:
    PPU();
    void initPPU();

    enum class Register {
        PPUCTRL,
        PPUMASK,
        PPUSTATUS,
        OAMADDR,
        OAMDATA,
        PPUSCROLL,
        PPUADDR,
        PPUDATA,
    };

    uint8_t view(uint8_t ppuRegister) const;
    uint8_t read(uint8_t ppuRegister);
    void write(uint8_t ppuRegister, uint8_t value);

    uint8_t ppuView(uint16_t address) const;
    uint8_t ppuRead(uint16_t address);
    void ppuWrite(uint16_t address, uint8_t value);

    Cartridge cartridge;

    // Width/height of an tile in the pattern table
    static constexpr uint16_t PATTERN_TABLE_TILE_SIZE = 0x8;

    // Width/height of pattern table, measured by number of tiles
    static constexpr uint16_t PATTERN_TABLE_NUM_TILES = 0x10;

    static constexpr uint16_t PATTERN_TABLE_SIZE = PATTERN_TABLE_TILE_SIZE * PATTERN_TABLE_NUM_TILES;

    static constexpr uint16_t PATTERN_TABLE_TILE_BYTES = (PATTERN_TABLE_TILE_SIZE * PATTERN_TABLE_TILE_SIZE * 2) / 8;
    static constexpr uint16_t PATTERN_TABLE_TOTAL_BYTES = PATTERN_TABLE_TILE_BYTES * PATTERN_TABLE_NUM_TILES * PATTERN_TABLE_NUM_TILES;

    using PatternTable = std::array<std::array<uint32_t, PATTERN_TABLE_SIZE>, PATTERN_TABLE_SIZE>;
    PatternTable getPatternTable(bool isBackground, uint8_t palleteNumber) const;

    std::array<uint32_t, 0x20> getPalleteRamColors() const;

    void executeCycle();

    using Display = std::array<std::array<uint32_t, 256>, 240>;
    std::unique_ptr<Display> finishedDisplay;

    static constexpr uint16_t OAM_BUFFER_SIZE = 0x100;
    static constexpr uint16_t OAM_SPRITES = OAM_BUFFER_SIZE / 4;
    using OAMBuffer = std::array<uint8_t, OAM_BUFFER_SIZE>;
    OAMBuffer oamBuffer;

    bool frameReadyFlag;

    bool nmiRequested();
    void clearNMIRequest();
    bool irqRequested();

    // Serialization
    void serialize(Serializer& s) const;
    void deserialize(Deserializer& d);

private:
    // PPU internal data structures (descriptions from https://www.nesdev.org/wiki/PPU_registers)

    // Controller ($2000) > write
    // Common name: PPUCTRL
    // Description: PPU control register
    // Access: write
    // Various flags controlling PPU operation
    // 7  bit  0
    // ---- ----
    // VPHB SINN
    // |||| ||||
    // |||| ||++- Base nametable address
    // |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
    // |||| |+--- VRAM address increment per CPU read/write of PPUDATA
    // |||| |     (0: add 1, going across; 1: add 32, going down)
    // |||| +---- Sprite pattern table address for 8x8 sprites
    // ||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
    // |||+------ Background pattern table address (0: $0000; 1: $1000)
    // ||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
    // |+-------- PPU master/slave select
    // |          (0: read backdrop from EXT pins; 1: output color on EXT pins)
    // +--------- Generate an NMI at the start of the
    //         vertical blanking interval (0: off; 1: on)
    struct Control {
        bool nametableX : 1;
        bool nametableY : 1;
        bool vramAddressIncrement : 1;
        bool spritePatternTable : 1;
        bool backgroundPatternTable : 1;
        bool spriteSize : 1;
        bool ppuSelect : 1;
        bool nmiEnabled : 1;

        Control() = default;
        Control(uint8_t data);
        uint8_t toUInt8() const;
        void setFromUInt8(uint8_t data);
    };

    // Mask ($2001) > write
    // Common name: PPUMASK
    // Description: PPU mask register
    // Access: write
    // This register controls the rendering of sprites and backgrounds, as well as colour effects.
    // 7  bit  0
    // ---- ----
    // BGRs bMmG
    // |||| ||||
    // |||| |||+- Greyscale (0: normal color, 1: produce a greyscale display)
    // |||| ||+-- 1: Show background in leftmost 8 pixels of screen, 0: Hide
    // |||| |+--- 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
    // |||| +---- 1: Show background
    // |||+------ 1: Show sprites
    // ||+------- Emphasize red (green on PAL/Dendy)
    // |+-------- Emphasize green (red on PAL/Dendy)
    // +--------- Emphasize blue
    struct Mask {
        bool greyscale : 1;
        bool showBackgroundLeft : 1;
        bool showSpritesLeft : 1;
        bool showBackground : 1;
        bool showSprites : 1;

        // Unused for now
        bool emphRed : 1;
        bool emphGreen : 1;
        bool emphBlue : 1;

        Mask() = default;
        Mask(uint8_t data);
        uint8_t toUInt8() const;
        void setFromUInt8(uint8_t data);
    };

    // Status ($2002) < read
    // Common name: PPUSTATUS
    // Description: PPU status register
    // Access: read
    // This register reflects the state of various functions inside the PPU. It is often used for determining timing. To determine when the PPU has reached a given pixel of the screen, put an opaque (non-transparent) pixel of sprite 0 there.
    // 7  bit  0
    // ---- ----
    // VSO. ....
    // |||| ||||
    // |||+-++++- PPU open bus. Returns stale PPU bus contents.
    // ||+------- Sprite overflow. The intent was for this flag to be set
    // ||         whenever more than eight sprites appear on a scanline, but a
    // ||         hardware bug causes the actual behavior to be more complicated
    // ||         and generate false positives as well as false negatives; see
    // ||         PPU sprite evaluation. This flag is set during sprite
    // ||         evaluation and cleared at dot 1 (the second dot) of the
    // ||         pre-render line.
    // |+-------- Sprite 0 Hit.  Set when a nonzero pixel of sprite 0 overlaps
    // |          a nonzero background pixel; cleared at dot 1 of the pre-render
    // |          line.  Used for raster timing.
    // +--------- Vertical blank has started (0: not in vblank; 1: in vblank).
    //         Set at dot 1 of line 241 (the line *after* the post-render
    //         line); cleared after reading $2002 and at dot 1 of the
    //         pre-render line.
    struct Status {
        uint8_t openBus : 5;
        bool spriteOverflow : 1;
        bool sprite0Hit : 1;
        bool vBlankStarted : 1;

        Status() = default;
        Status(uint8_t data);
        uint8_t toUInt8() const;
        void setFromUInt8(uint8_t data);
    };

    // https://www.nesdev.org/wiki/PPU_scrolling
    // The 15 bit registers t and v are composed this way during rendering:
    // yyy NN YYYYY XXXXX
    // ||| || ||||| +++++-- coarse X scroll
    // ||| || +++++-------- coarse Y scroll
    // ||| ++-------------- nametable select
    // +++----------------- fine Y scroll
    // Note that while the v register has 15 bits, the PPU memory space is only 14 bits wide. The highest bit is unused for access through $2007.
    struct InternalRegister {
        uint8_t coarseX : 5;
        uint8_t coarseY : 5;
        bool nametableX : 1;
        bool nametableY : 1;
        uint8_t fineY : 3;
        bool unused : 1;

        InternalRegister() = default;
        InternalRegister(uint16_t data);
        uint16_t toUInt16() const;
        void setFromUInt16(uint16_t data);
    };

    struct OAMEntry {
        uint8_t y;
        uint8_t tileIndex;
        uint8_t attributes;
        uint8_t x;

        OAMEntry() = default;
        OAMEntry(uint32_t data);
        OAMEntry(const OAMBuffer& oamBuffer, uint8_t bufferIndex);
        uint32_t toUInt32() const;
        void setFromUInt32(uint32_t data);
    };

    Control control;
    Mask mask;
    Status status;

    uint16_t getNameTableIndex(uint16_t address) const;

    // Some registers require two instructions to write the data, 
    // and so we store a boolean to represent which byte of the data we are currently writing
    bool addressLatch;

    // This is where we write addresses to in PPUSCROLL and PPUDATA (lo/hi byte is controlled by addressLatch)
    InternalRegister temporaryVramAddress;
    InternalRegister vramAddress;

    uint8_t fineX;

    // Reading PPU data takes two instruction cycles, so we store data that we haven't yet read here
    uint8_t ppuBusData;

    static constexpr MemoryRange PATTERN_TABLE_RANGE{ 0x0000, 0x1FFF };
    static constexpr MemoryRange NAMETABLE_RANGE{ 0x2000, /*0x2FFF*/ 0x3EFF };
    static constexpr MemoryRange PALLETE_RAM_RANGE{ 0x3F00, 0x3FFF };

    static constexpr uint16_t NUM_SCREEN_COLORS = 0x40;
    static const std::array<uint32_t, NUM_SCREEN_COLORS> SCREEN_COLORS;

    std::array<uint8_t, 0x20> palleteRam;
    uint8_t getPalleteRamIndex(uint16_t address, bool read) const;

    using NameTable = std::array<uint8_t, 2 * KB>;
    NameTable nameTable;

    int32_t scanline;
    int32_t cycle;
    bool oddFrame;

    // internal latches
    uint16_t patternTableLoShifter;
    uint16_t patternTableHiShifter;
    uint16_t attributeTableLoShifter;
    uint16_t attributeTableHiShifter;

    uint8_t nextNameTableByte;

    uint8_t nextPatternTableLo;
    uint8_t nextPatternTableHi;
    bool nextAttributeTableLo;
    bool nextAttributeTableHi;

    // Rendering helper functions
    void preRenderScanline();
    void visibleScanlines();
    void verticalBlankScanlines();

    void handleMapper4IRQ();

    void doRenderingPipeline();
    void doStandardFetchCycle();

    void fetchNameTableByte();
    void fetchAttributeTableByte();
    void fetchPatternTableByteLo();
    void fetchPatternTableByteHi();

    void drawPixel();

    void reloadShifters();
    void shiftShifters();

    bool isRenderingEnabled() const;

    void incrementCycle();
    void incrementCoarseX();
    void incrementY();

    struct SpriteData {
        OAMEntry oam;
        uint8_t patternTableLo;
        uint8_t patternTableHi;
    };

    static constexpr int MAX_SPRITES = 8;
    std::vector<SpriteData> currentScanlineSprites;
    bool sprite0OnCurrentScanline;
    void fillCurrentScanlineSprites();

    std::unique_ptr<Display> workingDisplay;

    uint8_t oamAddress;

    uint16_t getPalleteRamAddress(uint8_t backgroundTable, uint8_t patternTable) const;

    bool nmiRequest;
    bool irqRequest;
};

#endif // PPU_HPP