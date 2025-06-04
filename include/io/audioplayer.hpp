#ifndef AUDIOPLAYER_HPP
#define AUDIOPLAYER_HPP

#include "io/guitypes.hpp"
#include "io/threadsafeaudioqueue.hpp"

#include <cstdint>
#include <mutex>
#include <queue>

#include <QAudioFormat>
#include <QIODevice>
#include <QWidget>

class AudioPlayer : public QIODevice {
    Q_OBJECT

public:
    AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY>* audioSamples);
protected:
    int64_t readData(char* data, int64_t maxSize) override;
    int64_t writeData(const char* data, int64_t maxSize) override;
    int64_t bytesAvailable() const override;

private:
    QAudioFormat audioFormat;
    ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY>* audioSamples;
};

#endif // AUDIOPLAYER_HPP