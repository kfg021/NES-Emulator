#ifndef AUDIOPLAYER_HPP
#define AUDIOPLAYER_HPP

#include "io/guitypes.hpp"

#include <cstdint>
#include <mutex>
#include <queue>

#include <QAudioFormat>
#include <QIODevice>
#include <QWidget>

class AudioPlayer : public QIODevice {
    Q_OBJECT

public:
    AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, AudioQueue& audioSamples);
protected:
    int64_t readData(char* data, int64_t maxSize) override;
    int64_t writeData(const char* data, int64_t maxSize) override;
    int64_t bytesAvailable() const override;

private:
    QAudioFormat audioFormat;
    AudioQueue& audioSamples;
};

#endif // AUDIOPLAYER_HPP