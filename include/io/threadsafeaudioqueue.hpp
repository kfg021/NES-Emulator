#ifndef THREADSAFEAUDIOQUEUE_HPP
#define THREADSAFEAUDIOQUEUE_HPP

#include "util/circularbuffer.hpp"

#include <cstring>
#include <mutex>
#include <type_traits>

template <typename T, size_t Capacity, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
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
        return circBuffer.size();
    }

    void forcePush(const T& data) {
        // This evicts the oldest item in the audio queue if there are too many samples.
        // It keeps the audio in sync with the video but sometimes leads to audio glitches if audio is ahead
        std::lock_guard<std::mutex> guard(mtx);
        circBuffer.forcePush(data);
    }

    void erase() {
        std::lock_guard<std::mutex> guard(mtx);
        circBuffer.erase();
    }

    size_t popManyIntoBuffer(char* outputBuffer, size_t maxSize) {
        size_t maxEntries = maxSize / sizeof(T);
        size_t numEntries = 0;
        {
            std::lock_guard<std::mutex> guard(mtx);
            numEntries = std::min(circBuffer.size(), maxEntries);
            for (size_t i = 0; i < numEntries; i++) {
                size_t index = i * sizeof(T);
                T* address = reinterpret_cast<T*>(&outputBuffer[index]);
                T output = circBuffer.pop();
                std::memcpy(address, &output, sizeof(T));
            }
        }
        return numEntries * sizeof(T);
    }

private:
    CircularBuffer<T, Capacity> circBuffer;
    mutable std::mutex mtx;
};

#endif // THREADSAFEAUDIOQUEUE_HPP