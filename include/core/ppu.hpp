#ifndef PPU_HPP
#define PPU_HPP

#include "core/cartridge.hpp"
#include "util/serializer.hpp"
#include "util/util.hpp"

#include <memory>
#include <vector>

class PPU {
public:
    PPU(Cartridge& cartridge);
    void resetPPU();

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

    // Width/height of an tile in the pattern table
    static constexpr uint16_t PATTERN_TABLE_TILE_SIZE = 0x8;

    // Width/height of pattern table, measured by number of tiles
    static constexpr uint16_t PATTERN_TABLE_NUM_TILES = 0x10;

    static constexpr uint16_t PATTERN_TABLE_SIZE = PATTERN_TABLE_TILE_SIZE * PATTERN_TABLE_NUM_TILES;

    static constexpr uint16_t PATTERN_TABLE_TILE_BYTES = (PATTERN_TABLE_TILE_SIZE * PATTERN_TABLE_TILE_SIZE * 2) / 8;
    static constexpr uint16_t PATTERN_TABLE_TOTAL_BYTES = PATTERN_TABLE_TILE_BYTES * PATTERN_TABLE_NUM_TILES * PATTERN_TABLE_NUM_TILES;

    using PatternTable = std::array<std::array<uint32_t, PATTERN_TABLE_SIZE>, PATTERN_TABLE_SIZE>;
    struct PatternTables {
        PatternTable backgroundPatternTable;
        PatternTable spritePatternTable;
    };
    std::unique_ptr<PatternTables> getPatternTables(uint8_t backgroundPalleteNumber, uint8_t spritePalleteNumber) const;

    std::array<uint32_t, 0x20> getPalleteRamColors() const;

    void executeCycle();

    using Display = std::array<std::array<uint32_t, 256>, 240>;
    std::unique_ptr<Display> finishedDisplay;

    static constexpr uint16_t OAM_BUFFER_SIZE = 0x100;
    static constexpr uint16_t OAM_SPRITES = OAM_BUFFER_SIZE / 4;
    using OAMBuffer = std::array<uint8_t, OAM_BUFFER_SIZE>;
    OAMBuffer oamBuffer;

    bool frameReadyFlag;

    bool nmiRequested() const;
    void clearNMIRequest();
    bool irqRequested() const;

    // Serialization
    void serialize(Serializer& s) const;
    void deserialize(Deserializer& d);

private:
    Cartridge& cartridge;

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
        uint8_t data = 0;
        BitField<0, 1> nametableX{ data };
        BitField<1, 1> nametableY{ data };
        BitField<2, 1> vramAddressIncrement{ data };
        BitField<3, 1> spritePatternTable{ data };
        BitField<4, 1> backgroundPatternTable{ data };
        BitField<5, 1> spriteSize{ data };
        BitField<6, 1> ppuSelect{ data };
        BitField<7, 1> nmiEnabled{ data };
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
        uint8_t data;
        BitField<0, 1> greyscale{ data };
        BitField<1, 1> showBackgroundLeft{ data };
        BitField<2, 1> showSpritesLeft{ data };
        BitField<3, 1> showBackground{ data };
        BitField<4, 1> showSprites{ data };
        BitField<5, 1> emphRed{ data };
        BitField<6, 1> emphGreen{ data };
        BitField<7, 1> emphBlue{ data };
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
        uint8_t data;
        BitField<0, 5> openBus{ data };
        BitField<5, 1> spriteOverflow{ data };
        BitField<6, 1> sprite0Hit{ data };
        BitField<7, 1> vBlankStarted{ data };
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
        uint16_t data;
        BitField<0, 5, uint16_t>  coarseX{ data };
        BitField<5, 5, uint16_t>  coarseY{ data };
        BitField<10, 1, uint16_t> nametableX{ data };
        BitField<11, 1, uint16_t> nametableY{ data };
        BitField<12, 3, uint16_t> fineY{ data };
    };

    Control control;
    Mask mask;
    Status status;

    uint16_t getNameTableIndex(uint16_t address) const;
    uint8_t viewNameTable(uint16_t address) const;
    uint8_t readNameTable(uint16_t address);

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

    // Screen colors taken from https://www.nesdev.org/wiki/PPU_palettes
    static constexpr uint16_t NUM_SCREEN_COLORS = 0x40;
    static constexpr std::array<uint32_t, NUM_SCREEN_COLORS> SCREEN_COLORS = {
        0xFF626262, 0xFF001FB2, 0xFF2404C8, 0xFF5200B2, 0xFF730076, 0xFF800024, 0xFF730B00, 0xFF522800, 0xFF244400, 0xFF005700, 0xFF005C00, 0xFF005324, 0xFF003C76, 0xFF000000, 0xFF000000, 0xFF000000,
        0xFFABABAB, 0xFF0D57FF, 0xFF4B30FF, 0xFF8A13FF, 0xFFBC08D6, 0xFFD21269, 0xFFC72E00, 0xFF9D5400, 0xFF607B00, 0xFF209800, 0xFF00A300, 0xFF009942, 0xFF007DB4, 0xFF000000, 0xFF000000, 0xFF000000,
        0xFFFFFFFF, 0xFF53AEFF, 0xFF9085FF, 0xFFD365FF, 0xFFFF57FF, 0xFFFF5DCF, 0xFFFF7757, 0xFFFA9E00, 0xFFBDC700, 0xFF7AE700, 0xFF43F611, 0xFF26EF7E, 0xFF2CD5F6, 0xFF4E4E4E, 0xFF000000, 0xFF000000,
        0xFFFFFFFF, 0xFFB6E1FF, 0xFFCED1FF, 0xFFE9C3FF, 0xFFFFBCFF, 0xFFFFBDF4, 0xFFFFC6C3, 0xFFFFD59A, 0xFFE9E681, 0xFFCEF481, 0xFFB6FB9A, 0xFFA9FAC3, 0xFFA9F0F4, 0xFFB8B8B8, 0xFF000000, 0xFF000000,
    };

    std::array<uint8_t, 0x20> palleteRam;
    uint8_t getPalleteRamIndexRead(uint16_t address) const;
    uint8_t getPalleteRamIndexWrite(uint16_t address) const;

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

    struct OAMEntry {
        uint8_t y;
        uint8_t tileIndex;
        uint8_t attributes;
        uint8_t x;
    };

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
    uint8_t viewPalleteRam(uint16_t address) const;

    bool nmiRequest;
    bool irqRequest;

    static constexpr uint8_t NMI_DELAY_TIME = 3;
    uint8_t nmiDelayCounter;

    // Color tint bits (https://www.nesdev.org/wiki/NTSC_video)
    // Tests performed on NTSC NES show that emphasis does not affect the black colors in columns $E or $F, but it does affect all other columns, including the blacks and greys in column $D.
    // The terminated measurements above suggest that resulting attenuated absolute voltage is on average 0.816328 times the un-attenuated absolute voltage.
    // attenuated absolute = absolute * 0.816328
    static constexpr std::array<uint8_t, 3 * 256> ATTENUATION_TABLE = []() constexpr {
        constexpr float ATTENUATION = 0.816328f;
        
        std::array<uint8_t, 3 * 256> table{};
        
        float attenFactor = 1.0f;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 256; j++) {
                table[(i << 8) | j] = static_cast<uint8_t>(j * attenFactor);
            }
            attenFactor *= ATTENUATION;
        }

        return table;
    }();
};

#endif // PPU_HPP