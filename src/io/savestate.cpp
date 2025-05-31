#include "io/savestate.hpp"

#include "core/cpu.hpp"
#include "core/ppu.hpp"

#include <array>
#include <vector>

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QVector>

bool createSaveState(const std::string& filePath, const Bus& bus);
bool loadSaveState(const std::string& filePath, Bus& bus);

template <typename T, size_t size>
QDataStream& operator<<(QDataStream& out, const std::array<T, size>& state) {
    bool success = out.writeRawData(reinterpret_cast<const char*>(state.data()), size * sizeof(T));
    Q_ASSERT(success);
    return out;
}

template <typename T, size_t size>
QDataStream& operator>>(QDataStream& in, std::array<T, size>& state) {
    bool success = in.readRawData(reinterpret_cast<char*>(state.data()), size * sizeof(T));
    Q_ASSERT(success);
    return in;
}

QDataStream& operator<<(QDataStream& out, const CPU::State& state) {
    out << state.pc
        << state.a
        << state.x
        << state.y
        << state.sr
        << state.sp
        << state.remainingCycles
        << state.shouldAdvancePC;

    return out;
}

QDataStream& operator>>(QDataStream& in, CPU::State& state) {
    in >> state.pc
        >> state.a
        >> state.x
        >> state.y
        >> state.sr
        >> state.sp
        >> state.remainingCycles
        >> state.shouldAdvancePC;

    return in;
}

QDataStream& operator<<(QDataStream& out, const PPU::State& state) {
    out << state.oamBuffer
        << state.control
        << state.mask
        << state.status
        << state.addressLatch
        << state.temporaryVramAddress
        << state.vramAddress
        << state.fineX
        << state.ppuBusData
        << state.palleteRam
        << state.nameTable
        << state.scanline
        << state.cycle
        << state.frame
        << state.patternTableLoShifter
        << state.patternTableHiShifter
        << state.attributeTableLoShifter
        << state.attributeTableHiShifter
        << state.nextNameTableByte
        << state.nextPatternTableLo
        << state.nextPatternTableHi
        << state.nextAttributeTableLo
        << state.nextAttributeTableHi
        << state.oamAddress
        << state.nmiRequest
        << state.irqRequest;

    return out;
}

QDataStream& operator>>(QDataStream& in, PPU::State& state) {
    in >> state.oamBuffer
        >> state.control
        >> state.mask
        >> state.status
        >> state.addressLatch
        >> state.temporaryVramAddress
        >> state.vramAddress
        >> state.fineX
        >> state.ppuBusData
        >> state.palleteRam
        >> state.nameTable
        >> state.scanline
        >> state.cycle
        >> state.frame
        >> state.patternTableLoShifter
        >> state.patternTableHiShifter
        >> state.attributeTableLoShifter
        >> state.attributeTableHiShifter
        >> state.nextNameTableByte
        >> state.nextPatternTableLo
        >> state.nextPatternTableHi
        >> state.nextAttributeTableLo
        >> state.nextAttributeTableHi
        >> state.oamAddress
        >> state.nmiRequest
        >> state.irqRequest;

    return in;
}

// Save State format:
// Indentifier?
// Hash of ROM file
// Bus State
// CPU State
// PPU State
// APU State
// Mapper State

bool createSaveState(const std::string& filePath, const Bus& bus) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << file.errorString();
        return false;
    }

    QDataStream out(&file);
    out << bus.cpu->getState();
    out << bus.ppu->getState();

    return true;
}

bool loadSaveState(const std::string& filePath, Bus& bus) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for reading:" << file.errorString();
        return false;
    }

    QDataStream in(&file);

    CPU::State cpuState;
    in >> cpuState;
    bus.cpu->restoreState(cpuState);

    PPU::State ppuState;
    in >> ppuState;
    bus.ppu->restoreState(ppuState);

    return true;
}