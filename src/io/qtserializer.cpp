#include "io/qtserializer.hpp"

#include <QFile>
#include <QIODevice>

QtSerializer::QtSerializer() {
    out.setVersion(QDataStream::Qt_6_0);
    out.setByteOrder(QDataStream::BigEndian);
}

bool QtSerializer::good() {
    return out.device() && (out.status() == QDataStream::Ok);
}

bool QtSerializer::openFile(const QString& filePath) {
    if (filePath.isEmpty()) return false;

    if (file.isOpen()) file.close();
    out.setDevice(nullptr);

    file.setFileName(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    out.setDevice(&file);
    return true;
}

void QtSerializer::serializeUInt8(uint8_t data) { if (good()) out << data; }
void QtSerializer::serializeUInt16(uint16_t data) { if (good()) out << data; }
void QtSerializer::serializeUInt32(uint32_t data) { if (good()) out << data; }
void QtSerializer::serializeUInt64(uint64_t data) { if (good()) out << data; }
void QtSerializer::serializeInt32(int32_t data) { if (good()) out << data; }


QtDeserializer::QtDeserializer() {
    in.setVersion(QDataStream::Qt_6_0);
    in.setByteOrder(QDataStream::BigEndian);
}

bool QtDeserializer::good() {
    return in.device() && (in.status() == QDataStream::Ok);
}

bool QtDeserializer::openFile(const QString& filePath) {
    if (filePath.isEmpty()) return false;

    if (file.isOpen()) file.close();
    in.setDevice(nullptr);

    file.setFileName(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    in.setDevice(&file);
    return true;
}

void QtDeserializer::deserializeUInt8(uint8_t& data) { if (good()) in >> data; };
void QtDeserializer::deserializeUInt16(uint16_t& data) { if (good()) in >> data; };
void QtDeserializer::deserializeUInt32(uint32_t& data) { if (good())in >> data; };
void QtDeserializer::deserializeUInt64(uint64_t& data) { if (good()) in >> data; };
void QtDeserializer::deserializeInt32(int32_t& data) { if (good()) in >> data; };