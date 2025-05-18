#include "gui/audioplayer.hpp"

AudioPlayer::AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, bool muted, ThreadSafeQueue<float>* audioSamples)
    : QIODevice(parent),
    audioFormat(audioFormat),
    muted(muted),
    audioSamples(audioSamples) {

    open(QIODevice::ReadOnly);
}

int64_t AudioPlayer::readData(char* data, int64_t maxSize) {
    // return audioSamples->popManyIntoBuffer(data, maxSize);
    size_t amt = audioSamples->popManyIntoBuffer(data, maxSize);
    // if (amt < maxSize) {
    //     qDebug() << "requesting " << maxSize << " " << "bytes of audio, only got: " << amt << "!";
    // }
    // else if (maxSize > 0 && maxSize < 16384){
    //     qDebug() << "only requesting " << maxSize << " " << "bytes!";
    // }
    size_t s = audioSamples->size();
    if (s) {
        qDebug() << "queue has " << s << " samples left!";
    }
    return amt;
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