#ifndef QTSERIALIZER_HPP
#define QTSERIALIZER_HPP

#include "util/serializer.hpp"

#include <QDataStream>
#include <QFile>
#include <QString>

class QtSerializer : public Serializer {
public:
    QtSerializer();

    bool good();
    bool openFile(const QString& filePath);

    void serializeUInt8(uint8_t data) override;
    void serializeUInt16(uint16_t data) override;
    void serializeUInt32(uint32_t data) override;
    void serializeUInt64(uint64_t data) override;
    void serializeInt32(int32_t data) override;

private:
    QFile file;
    QDataStream out;
};

class QtDeserializer : public Deserializer {
public:
    QtDeserializer();

    bool good();
    bool openFile(const QString& filePath);

    void deserializeUInt8(uint8_t& data) override;
    void deserializeUInt16(uint16_t& data) override;
    void deserializeUInt32(uint32_t& data) override;
    void deserializeUInt64(uint64_t& data) override;
    void deserializeInt32(int32_t& data) override;

private:
    QFile file;
    QDataStream in;
};

#endif // QTSERIALIZER_HPP