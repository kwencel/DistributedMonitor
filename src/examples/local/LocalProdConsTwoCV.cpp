#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

#define MAX_QUEUE_SIZE 5

template<typename T>
class ConsumerProducerTwoCondTest {
    std::size_t maxSize;
    std::condition_variable queueEmptyCv;
    std::condition_variable queueFullCv;
    std::mutex mutex;
    std::queue<T> queue;

public:
    explicit ConsumerProducerTwoCondTest(std::size_t size) : maxSize(size) { }

    void produce(T request) {
        std::unique_lock<std::mutex> lock(mutex);
        queueFullCv.wait(lock, [&](){ return not isFull(); });
        queue.push(request);
        std::cout << "Produced " << std::to_string(request) << std::endl;
        lock.unlock();
        queueEmptyCv.notify_one();
    }

    T consume() {
        std::unique_lock<std::mutex> lock(mutex);
        queueEmptyCv.wait(lock, [&](){ return not isEmpty(); });
        T request = queue.front();
        std::cout << "Consumed " << std::to_string(request) << std::endl;
        queue.pop();
        lock.unlock();
        queueFullCv.notify_one();
        return request;
    }

    bool isFull() const {
        return queue.size() >= maxSize;
    }

    bool isEmpty() const {
        return queue.empty();
    }
};

int main() {
    ConsumerProducerTwoCondTest<unsigned long> queue(MAX_QUEUE_SIZE);

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