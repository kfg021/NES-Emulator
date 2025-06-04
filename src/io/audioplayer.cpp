#include "io/audioplayer.hpp"

AudioPlayer::AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY>* audioSamples)
    : QIODevice(parent),
    audioFormat(audioFormat),
    audioSamples(audioSamples) {

    open(QIODevice::ReadOnly);
}

int64_t AudioPlayer::readData(char* data, int64_t maxSize) {
    // Try to fill the buffer with bytes if we have any
    // If not the audio is falling behind a bit, so sill the entire buffer with 0s so we can catch up
    size_t bytesFilled = audioSamples->popManyIntoBuffer(data, maxSize);
    if (bytesFilled) {
        return bytesFilled;
    }
    else {
        std::memset(data, 0x00, maxSize);
        return maxSize;
    }
}

int64_t AudioPlayer::writeData(const char* /*data*/, int64_t /*maxSize*/) {
    return -1;
}

int64_t AudioPlayer::bytesAvailable() const {
    size_t bytesInQueue = audioSamples->size() * sizeof(float);
    if (bytesInQueue) {
        return bytesInQueue;
    }
    else {
        return AUDIO_QUEUE_MAX_CAPACITY * sizeof(float);
    }
}