#ifndef SAVESTATE_HPP
#define SAVESTATE_HPP

#include "core/bus.hpp"

#include <array>
#include <optional>

#include <QString>

class SaveState {
public:
    SaveState(Bus& bus, const QString& romFilePath);

    struct CreateStatus {
        enum class Code {
            SUCCESS,
            INVALID_FILE,
            HASH_ERROR,
            WRITING_ERROR
        };

        Code code;
        QString message;
    };

    struct LoadStatus {
        enum class Code {
            SUCCESS,
            INVALID_FILE,
            INVALID_FORMAT,
            INVALID_VERSION,
            HASH_ERROR,
            READING_ERROR,
        };

        Code code;
        QString message;
    };

    CreateStatus createSaveState(const QString& filePath) const;
    LoadStatus loadSaveState(const QString& filePath);

private:
    // SAVE STATE FORMAT

    // File extension:
    //      .sstate

    // Contents:
    //      Format ID
    //      Save state major version
    //      Save state minor version
    //      Save state patch
    //      Hash of ROM file
    //      Bus state
    //      CPU state
    //      PPU state
    //      APU state
    //      Mapper state

    // All save state files must begin with this ID to distinguish them from other binary data files
    static constexpr uint32_t FORMAT_ID = 0xABCD1234;

    static constexpr uint8_t VERSION_MAJOR = 1;
    static constexpr uint8_t VERSION_MINOR = 0;
    static constexpr uint8_t VERSION_PATCH = 0;

    static constexpr int HASH_BYTES = 32;
    using RomHash = std::array<uint8_t, HASH_BYTES>;
    std::optional<RomHash> createRomHash(const QString& filePath) const;
    const std::optional<RomHash> ROM_HASH;

    Bus& bus;
};



#endif // SAVESTATE_HPP