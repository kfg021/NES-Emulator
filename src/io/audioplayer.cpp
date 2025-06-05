#include "io/audioplayer.hpp"

AudioPlayer::AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY>* audioSamples)
    : QIODevice(parent),
    audioFormat(audioFormat),
    audioSamples(audioSamples) {

    open(QIODevice::ReadOnly);
}

int64_t AudioPlayer::readData(char* data, int64_t maxSize) {
    return audioSamples->popManyIntoBuffer(data, maxSize);
}

int64_t AudioPlayer::writeData(const char* /*data*/, int64_t /*maxSize*/) {
    return -1;
}

int64_t AudioPlayer::bytesAvailable() const {
    return audioSamples->size() * sizeof(float);
}