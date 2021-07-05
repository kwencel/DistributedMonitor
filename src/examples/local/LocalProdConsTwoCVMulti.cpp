#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

#define MAX_QUEUE_SIZE 5

template<typename T>
class ConsumerProducerTwoCondMultiTest {
    std::size_t maxSize;
    std::condition_variable queueEmptyCv;
    std::condition_variable queueFullCv;
    std::mutex mutex;
    std::queue<T> queue;

public:
    explicit ConsumerProducerTwoCondMultiTest(std::size_t size) : maxSize(size) { }

    void produce(T request) {
        std::unique_lock<std::mutex> lock(mutex);
        queueFullCv.wait(lock, [&](){ return not isFull(); });
        queue.push(request);
        std::cout << "[Thread " << std::this_thread::get_id() << " produced " << std::to_string(request) << std::endl;
        lock.unlock();
        queueEmptyCv.notify_all();
    }

    T consume() {
        std::unique_lock<std::mutex> lock(mutex);
        queueEmptyCv.wait(lock, [&](){ return not isEmpty(); });
        T request = queue.front();
        std::cout << "[Thread " << std::this_thread::get_id() << " consumed " << std::to_string(request) << std::endl;
        queue.pop();
        lock.unlock();
        queueFullCv.notify_all();
        return request;
    }

    [[nodiscard]] bool isFull() const {
        return queue.size() >= maxSize;
    }

    [[nodiscard]] bool isEmpty() const {
        return queue.empty();
    }
};

template <typename T>
void producerFunc(ConsumerProducerTwoCondMultiTest<T>* queue) {
    unsigned long i = 0;
    while (true) {
        queue->produce(i);
        i = (i + 1) % MAX_QUEUE_SIZE;
    }
}

template <typename T>
void consumerFunc(ConsumerProducerTwoCondMultiTest<T>* queue) {
    while (true) {
        queue->consume();
    }
}

int main() {
    ConsumerProducerTwoCondMultiTest<unsigned long> queue(MAX_QUEUE_SIZE);

    std::thread producer1(producerFunc<unsigned long>, &queue);
    std::thread producer2(producerFunc<unsigned long>, &queue);
    std::thread producer3(producerFunc<unsigned long>, &queue);
    std::thread producer4(producerFunc<unsigned long>, &queue);

    std::thread consumer1(consumerFunc<unsigned long>, &queue);
    std::thread consumer2(consumerFunc<unsigned long>, &queue);
    std::thread consumer3(consumerFunc<unsigned long>, &queue);
    consumerFunc(&queue);
}