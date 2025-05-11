#include "gui/audioplayer.hpp"

AudioPlayer::AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, bool muted)
    : QIODevice(parent),
    audioFormat(audioFormat),
    muted(muted) {

    open(QIODevice::ReadOnly);
}

int64_t AudioPlayer::readData(char* data, int64_t maxSize) {
    int maxSamples = maxSize / sizeof(float);
    int numSamples = 0;

    {
        std::lock_guard<std::mutex> guard(mtx);
        numSamples = std::min(static_cast<int>(audioSamples.size()), maxSamples);
        for (int i = 0; i < numSamples; i++) {
            float sample = audioSamples.front();
            audioSamples.pop();

            int index = i * sizeof(float);
            float* address = reinterpret_cast<float*>(&data[index]);
            std::memcpy(address, &sample, sizeof(float));
        }
    }

    return numSamples * sizeof(float);
}

int64_t AudioPlayer::writeData(const char* /*data*/, int64_t /*maxSize*/) {
    return -1;
}

int64_t AudioPlayer::bytesAvailable() const {
    std::lock_guard<std::mutex> guard(mtx);
    return audioSamples.size() * sizeof(float);
}

void AudioPlayer::addSample(float sample) {
    if (!muted) {
        std::lock_guard<std::mutex> guard(mtx);
        audioSamples.push(sample);
    }
}

void AudioPlayer::mute() {
    if (!muted) {
        muted = true;
        {
            std::lock_guard<std::mutex> guard(mtx);

            std::queue<float> empty;
            std::swap(audioSamples, empty);
        }
    }
}

void AudioPlayer::unmute() {
    if (muted) {
        muted = false;
    }
}