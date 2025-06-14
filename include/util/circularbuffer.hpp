#ifndef CIRCULARBUFFER_HPP
#define CIRCULARBUFFER_HPP

#include <array>
#include <type_traits>

template <typename T, size_t Capacity, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
class CircularBuffer {
public:
    CircularBuffer() {
        buffer = {};
        readPointer = 0;
        writePointer = 0;
        currentSize = 0;
    }

    ~CircularBuffer() = default;

    size_t size() const {
        return currentSize;
    }

    T front() const {
        return buffer[readPointer];
    }

    void forcePush(const T& data) {
        // This evicts the oldest item in the buffer if there are too many entries
        if (currentSize == Capacity) {
            popInternal();
        }

        pushInternal(data);
    }


    T pop() {
        T output = buffer[readPointer];
        popInternal();
        return output;
    }

    void erase() {
        readPointer = 0;
        writePointer = 0;
        currentSize = 0;
    }

private:
    std::array<T, Capacity> buffer;
    size_t readPointer;
    size_t writePointer;
    size_t currentSize;

    void pushInternal(const T& data) {
        buffer[writePointer] = data;
        writePointer++;
        currentSize++;
        if (writePointer == Capacity) writePointer = 0;
    }

    void popInternal() {
        readPointer++;
        currentSize--;
        if (readPointer == Capacity) readPointer = 0;
    }
};

#endif // CIRCULARBUFFER_HPP