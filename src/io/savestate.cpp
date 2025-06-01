#include "io/savestate.hpp"

#include "core/cpu.hpp"
#include "core/ppu.hpp"
#include "io/qtserializer.hpp"

#include <array>
#include <vector>

#include <QFileDialog>
#include <QDir>

// SAVE STATE FORMAT:
// Format ID
// Save state major version
// Save state minor version
// Save state patch
// MD5 hash of ROM file
// Bus state
// CPU state
// PPU state
// APU state
// Mapper state

static constexpr uint32_t FORMAT_ID = 0xABCD1234;

static constexpr uint8_t VERSION_MAJOR = 1;
static constexpr uint8_t VERSION_MINOR = 0;
static constexpr uint8_t VERSION_PATCH = 0;

bool createSaveState(const Bus& bus) {
    QtSerializer s;
    QString filePath = QFileDialog::getSaveFileName(nullptr, "Create save state", QDir::homePath(), "(*.bin)");
    if (s.openFile(filePath)) {
        s.serializeUInt32(FORMAT_ID);
        s.serializeUInt8(VERSION_MAJOR);
        s.serializeUInt8(VERSION_MINOR);
        s.serializeUInt8(VERSION_PATCH);
        // s.serializeArray();

        bus.serialize(s);
        bus.cpu->serialize(s);
        bus.ppu->serialize(s);
        // bus.apu->serialize(s);
        bus.ppu->cartridge.mapper->serialize(s);

        return true;
    }
    else {
        return false;
    }
}

bool loadSaveState(Bus& bus) {
    struct Header {
        uint32_t formatID;
        uint8_t versionMajor;
        uint8_t versionMinor;
        uint8_t versionPatch;
    };

    QtDeserializer d;
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Load save state", QDir::homePath(), "(*.bin)");
    if (d.openFile(filePath)) {
        // TODO: Check header
        Header header;
        d.deserializeUInt32(header.formatID);
        d.deserializeUInt8(header.versionMajor);
        d.deserializeUInt8(header.versionMinor);
        d.deserializeUInt8(header.versionPatch);
        // d.deserializeArray();

        bus.deserialize(d);
        bus.cpu->deserialize(d);
        bus.ppu->deserialize(d);
        // bus.apu->deserialize(d);
        bus.ppu->cartridge.mapper->deserialize(d);

        return true;
    }
    else {
        return false;
    }
}