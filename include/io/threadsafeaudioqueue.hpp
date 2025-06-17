#ifndef THREADSAFEAUDIOQUEUE_HPP
#define THREADSAFEAUDIOQUEUE_HPP

#include "util/circularbuffer.hpp"

#include <cstring>
#include <mutex>
#include <type_traits>

template <size_t Capacity>
class ThreadSafeAudioQueue {
public:
    ThreadSafeAudioQueue() = default;
    ~ThreadSafeAudioQueue() = default;

    ThreadSafeAudioQueue(const ThreadSafeAudioQueue&) = delete;
    ThreadSafeAudioQueue& operator=(const ThreadSafeAudioQueue&) = delete;
    ThreadSafeAudioQueue(ThreadSafeAudioQueue&&) = delete;
    ThreadSafeAudioQueue& operator=(ThreadSafeAudioQueue&&) = delete;

    size_t size() const {
        std::lock_guard<std::mutex> guard(mtx);
        return audioBuffer.size();
    }

    void forcePush(float data) {
        // This evicts the oldest item in the audio queue if there are too many samples.
        // It keeps the audio in sync with the video but sometimes leads to audio glitches if audio is ahead
        std::lock_guard<std::mutex> guard(mtx);
        audioBuffer.forcePush(data);
    }

    void erase() {
        std::lock_guard<std::mutex> guard(mtx);
        audioBuffer.erase();
    }

    size_t popManyIntoBuffer(char* outputBuffer, size_t maxSize) {
        size_t maxEntries = maxSize / sizeof(float);
        size_t numEntries = 0;
        {
            std::lock_guard<std::mutex> guard(mtx);
            numEntries = std::min(audioBuffer.size(), maxEntries);
            for (size_t i = 0; i < numEntries; i++) {
                size_t index = i * sizeof(float);
                float* address = reinterpret_cast<float*>(&outputBuffer[index]);
                float output = audioBuffer.pop();
                std::memcpy(address, &output, sizeof(float));
            }
        }
        return numEntries * sizeof(float);
    }

private:
    CircularBuffer<float, Capacity> audioBuffer;
    mutable std::mutex mtx;
};

#endif // THREADSAFEAUDIOQUEUE_HPP