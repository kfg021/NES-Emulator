#include "gui/audioplayer.hpp"

AudioPlayer::AudioPlayer(QWidget* parent, const QAudioFormat& audioFormat, bool muted, ThreadSafeQueue<float>* queue)
    : QIODevice(parent),
        audioFormat(audioFormat),
        muted(muted),
        queue(queue) {

        open(QIODevice::ReadOnly);
    }

    int64_t AudioPlayer::readData(char* data, int64_t maxSize) {      
        return queue->popManyIntoBuffer(data, maxSize);
        // size_t amt = queue->popManyIntoBuffer(data, maxSize);
        // qDebug() << "requesting " << maxSize << " " << "bytes of audio, got: " << amt << "\n";
        // return amt;
    }

    int64_t AudioPlayer::writeData(const char* /*data*/, int64_t /*maxSize*/) {
        return -1;
    }

    int64_t AudioPlayer::bytesAvailable() const {
        return queue->size() * sizeof(float);
    }

    void AudioPlayer::tryToMute() {
        if (!muted) {
            muted = true;
            queue->erase();
        }
    }

    void AudioPlayer::tryToUnmute() {
        if (muted) {
            muted = false;
        }
    }