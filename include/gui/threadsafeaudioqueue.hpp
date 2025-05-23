#ifndef THREADSAFEAUDIOQUEUE_HPP
#define THREADSAFEAUDIOQUEUE_HPP

#include <array>
#include <cstring>
#include <mutex>
#include <optional>
#include <type_traits>

template <typename T, size_t capacity, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
class ThreadSafeAudioQueue {
public:
    ThreadSafeAudioQueue() {
        buffer = {};
        readPointer = 0;
        writePointer = 0;
        currentSize = 0;
    }

    ~ThreadSafeAudioQueue() = default;

    ThreadSafeAudioQueue(const ThreadSafeAudioQueue&) = delete;
    ThreadSafeAudioQueue& operator=(const ThreadSafeAudioQueue&) = delete;
    ThreadSafeAudioQueue(ThreadSafeAudioQueue&&) = delete;
    ThreadSafeAudioQueue& operator=(ThreadSafeAudioQueue&&) = delete;

    size_t size() const {
        std::lock_guard<std::mutex> guard(mtx);
        return currentSize;
    }

    void push(const T& data) {
        std::lock_guard<std::mutex> guard(mtx);

        // This evicts the oldest item in the audio queue if there are too many samples.
        // It keeps the audio in sync with the video but sometimes leads to audio glitches
        // TODO: More robust audio/video sync method
        if (currentSize == capacity) {
            popInternal();
        }
        
        pushInternal(data);
    }

    std::optional<T> front() const {
        {
            std::lock_guard<std::mutex> guard(mtx);
            if (currentSize) {
                return buffer[readPointer];
            }
        }

        return std::nullopt;
    }

    std::optional<T> pop() {
        {
            std::lock_guard<std::mutex> guard(mtx);
            if (currentSize) {
                T data = std::move(buffer[readPointer]);
                popInternal();
                return data;
            }
        }

        return std::nullopt;
    }

    void erase() {
        std::lock_guard<std::mutex> guard(mtx);
        readPointer = 0;
        writePointer = 0;
        currentSize = 0;
    }

    size_t popManyIntoBuffer(char* outputBuffer, size_t maxSize) {
        size_t maxEntries = maxSize / sizeof(T);
        size_t numEntries = 0;
        {
            std::lock_guard<std::mutex> guard(mtx);
            numEntries = std::min(currentSize, maxEntries);
            for (size_t i = 0; i < numEntries; i++) {
                size_t index = i * sizeof(T);
                T* address = reinterpret_cast<T*>(&outputBuffer[index]);
                std::memcpy(address, &buffer[readPointer], sizeof(T));
                popInternal();
            }
        }
        return numEntries * sizeof(T);
    }

private:
    std::array<T, capacity> buffer;
    size_t readPointer;
    size_t writePointer;
    size_t currentSize;
    mutable std::mutex mtx;

    void pushInternal(const T& data) {
        buffer[writePointer] = data;
        writePointer++;
        currentSize++;
        if (writePointer == capacity) writePointer = 0;
    }

    void popInternal() {
        readPointer++;
        currentSize--;
        if (readPointer == capacity) readPointer = 0;
    }
};

#endif // THREADSAFEAUDIOQUEUE_HPP