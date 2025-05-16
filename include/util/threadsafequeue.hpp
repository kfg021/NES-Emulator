#ifndef THREADSAFEQUEUE_HPP
#define THREADSAFEQUEUE_HPP

#include <mutex>
#include <optional>
#include <queue>
#include <type_traits>

template <typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

    size_t size() const {
        size_t queueSize = 0;
        {
            std::lock_guard<std::mutex> guard(mtx);
            queueSize = queue.size();
        }

        return queueSize;
    }

    void push(const T& data) {
        {
            std::lock_guard<std::mutex> guard(mtx);
            queue.push(data);
        }
    }

    std::optional<T> front() const {
        {
            std::lock_guard<std::mutex> guard(mtx);
            if (!queue.empty()) return queue.front();
        }
        return std::nullopt;
    }

    std::optional<T> pop() {
        {
            std::lock_guard<std::mutex> guard(mtx);
            if (!queue.empty()) {
                T frontData = queue.front();
                queue.pop();
                return frontData;
            }
        }

        return std::nullopt;
    }

    void erase() {
        {
            std::lock_guard<std::mutex> guard(mtx);
            std::queue<T> empty;
            std::swap(queue, empty);
        }
    }

    size_t popManyIntoBuffer(char* outputBuffer, size_t maxSize) {
        size_t maxEntries = maxSize / sizeof(T);
        size_t numEntries = 0;
        {
            std::lock_guard<std::mutex> guard(mtx);
            numEntries = std::min(queue.size(), maxSize);
            for (int i = 0; i < numEntries; i++) {
                T item = queue.front();
                queue.pop();

                int index = i * sizeof(T);
                T* address = reinterpret_cast<T*>(&outputBuffer[index]);
                std::memcpy(address, &item, sizeof(T));
            }
        }
        return numEntries * sizeof(T);
    }

private:
    std::queue<T> queue;
    mutable std::mutex mtx;
};

#endif // THREADSAFEQUEUE_HPP