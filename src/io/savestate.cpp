#include "io/savestate.hpp"

#include "core/cpu.hpp"
#include "core/ppu.hpp"
#include "io/qtserializer.hpp"

#include <QByteArray>
#include <QByteArrayView>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QString>

SaveState::SaveState(Bus& bus, const QString& romFilePath) :
    bus(bus),
    ROM_HASH(createRomHash(romFilePath)) {
}

std::optional<SaveState::RomHash> SaveState::createRomHash(const QString& romFilePath) const {
    QFile romFile(romFilePath);
    if (!romFile.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&romFile)) {
        return std::nullopt;
    }

    QByteArray result = hash.result();
    if (result.size() != HASH_BYTES) {
        return std::nullopt;
    }

    RomHash romHash;
    for (int i = 0; i < HASH_BYTES; i++) {
        romHash[i] = result[i];
    }
    return romHash;
}

SaveState::CreateStatus SaveState::createSaveState(const QString& saveFilePath) const {
    static const QString ERROR_MESSAGE_START = "Failed to create save state: ";

    if (!ROM_HASH.has_value()) {
        return {
            CreateStatus::Code::HASH_ERROR,
            ERROR_MESSAGE_START + "Could not hash current ROM file."
        };
    }

    QtSerializer s;
    if (s.openFile(saveFilePath)) {
        s.serializeUInt32(FORMAT_ID);
        s.serializeUInt8(VERSION_MAJOR);
        s.serializeUInt8(VERSION_MINOR);
        s.serializeUInt8(VERSION_PATCH);
        s.serializeArray(ROM_HASH.value(), s.uInt8Func);

        s.version = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH };

        bus.serialize(s);
        bus.cpu->serialize(s);
        bus.ppu->serialize(s);
        bus.apu->serialize(s);
        bus.cartridge->mapper->serialize(s);

        if (!s.good()) {
            return {
                CreateStatus::Code::WRITING_ERROR,
                ERROR_MESSAGE_START + "Error writing to file."
            };
        }

        return {
            CreateStatus::Code::SUCCESS,
            "Successfully created save state."
        };
    }
    else {
        return {
            CreateStatus::Code::INVALID_FILE,
            ERROR_MESSAGE_START + "Could not create file."
        };
    }
}

SaveState::LoadStatus SaveState::loadSaveState(const QString& filePath) {
    static const QString ERROR_MESSAGE_START = "Failed to load save state: ";

    struct Header {
        uint32_t formatID;
        uint8_t versionMajor;
        uint8_t versionMinor;
        uint8_t versionPatch;
        RomHash romHash;
    };

    if (!ROM_HASH.has_value()) {
        return {
            LoadStatus::Code::HASH_ERROR,
            ERROR_MESSAGE_START + "Could not hash current ROM file."
        };
    }

    QtDeserializer d;
    if (d.openFile(filePath)) {
        Header header;

        d.deserializeUInt32(header.formatID);
        if (header.formatID != FORMAT_ID) {
            return {
                LoadStatus::Code::INVALID_FORMAT,
                ERROR_MESSAGE_START + "File is not of the correct format."
            };
        }

        d.deserializeUInt8(header.versionMajor);
        d.deserializeUInt8(header.versionMinor);
        d.deserializeUInt8(header.versionPatch);
        if (header.versionMajor != VERSION_MAJOR) {
            return {
                LoadStatus::Code::INVALID_VERSION,
                ERROR_MESSAGE_START + "Save state major version does not match current major version."
            };
        }

        d.deserializeArray(header.romHash, d.uInt8Func);
        if (header.romHash != ROM_HASH.value()) {
            return {
                LoadStatus::Code::HASH_ERROR,
                ERROR_MESSAGE_START + "ROM hash from save state does not match current ROM hash."
            };
        }

        d.version = { header.versionMajor, header.versionMinor, header.versionPatch };

        bus.deserialize(d);
        bus.cpu->deserialize(d);
        bus.ppu->deserialize(d);
        bus.apu->deserialize(d);
        bus.cartridge->mapper->deserialize(d);

        // TODO: More robust save file checking
        if (!d.good()) {
            return {
                LoadStatus::Code::READING_ERROR,
                ERROR_MESSAGE_START + "Error reading from file. The save file might be corrupted."
            };
        }

        return {
            LoadStatus::Code::SUCCESS,
            "Successfully loaded save state."
        };
    }
    else {
        return {
            LoadStatus::Code::INVALID_FILE,
            ERROR_MESSAGE_START + "Could not open file."
        };
    }
}