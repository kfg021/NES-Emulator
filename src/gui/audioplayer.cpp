#include "gui/audioplayer.hpp"

AudioPlayer::AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, bool muted, ThreadSafeAudioQueue<float, AUDIO_QUEUE_MAX_CAPACITY>* audioSamples)
    : QIODevice(parent),
    audioFormat(audioFormat),
    muted(muted),
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

void AudioPlayer::tryToMute() {
    if (!muted) {
        muted = true;
        audioSamples->erase();
    }
}

void AudioPlayer::tryToUnmute() {
    if (muted) {
        muted = false;
    }
}