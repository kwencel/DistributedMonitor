#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

#define MAX_QUEUE_SIZE 5

/**
 * An example of a Producer-Consumer problem using only C++ threading to simulate different remote processes.
 * It could be used as a reference compared to the distributed variations to show how similar the interface is.
 */
template<typename T>
class ConsumerProducerOneCondTest {
    std::size_t maxSize;
    std::condition_variable cv;
    std::mutex mutex;
    std::queue<T> queue;

public:
    explicit ConsumerProducerOneCondTest(std::size_t size) : maxSize(size) { }

    void produce(T request) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]() { return not isFull(); });
        queue.push(request);
        std::cout << "Produced " << std::to_string(request) << std::endl;
        lock.unlock();
        cv.notify_all();
    }

    T consume() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]() { return not isEmpty(); });
        T request = queue.front();
        std::cout << "Consumed " << std::to_string(request) << std::endl;
        queue.pop();
        lock.unlock();
        cv.notify_all();
        return request;
    }

    [[nodiscard]] bool isFull() const {
        return queue.size() >= maxSize;
    }

    [[nodiscard]] bool isEmpty() const {
        return queue.empty();
    }
};

int main() {
    ConsumerProducerOneCondTest<unsigned long> queue(MAX_QUEUE_SIZE);

    std::thread producer([&]() {
        unsigned long i = 0;
        while (true) {
            queue.produce(i);
            i = (i + 1) % MAX_QUEUE_SIZE;
        }
    });

    while (true) {
        queue.consume();
    }
}