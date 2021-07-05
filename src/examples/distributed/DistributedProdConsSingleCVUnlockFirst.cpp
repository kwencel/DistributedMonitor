#include <queue>
#include <communication/MpiSimpleCommunicator.h>
#include <distributed/DistributedMonitor.h>
#include <distributed/DistributedConditionVariable.h>
#include "BoostSerializer.h"

#include <boost/serialization/deque.hpp>
#include <boost/serialization/queue.hpp>

#define MAX_QUEUE_SIZE 5

/**
 * In this example I present a Producer-Consumer problem using one CV with unlocking the mutex before notifying on CV.
 * It could be considered a form of optimization as in most CV implementations notifying on CV that have mutex already
 * unlocked yields slightly better performance.
 *
 * It can be achieved by creating an artificial scope for the variable 'sync'. It will be destroyed just before the
 * notify operation which will cause synchronization and mutex unlock messages to be sent.
 *
 * In other examples I don't use this optimization to keep the code more concise.
 */
template <typename T>
class BufferMonitor : public DistributedMonitor {
public:

    BufferMonitor(const std::string& name,
                  const std::shared_ptr<CommunicationManager>& communicationManager,
                  const std::shared_ptr<IDistributedExclusionAlgorithm>& mutexAlgorithm,
                  const std::shared_ptr<IDistributedConditionVariableAlgorithm>& cvAlgorithm,
                  std::size_t queueSize)

            : DistributedMonitor(name, communicationManager, mutexAlgorithm), maxSize(queueSize), cv(name, cvAlgorithm) { };

    std::string saveState() override {
        return BoostSerializer::serialize<std::queue<T>>(queue);
    }

    void restoreState(const std::string_view state) override {
        queue = BoostSerializer::deserialize<std::queue<T>>(state);
    }

    void produce(T request) {
        {
            auto sync = synchronized();
            cv.wait(mutex, [&]() { return not isFull(); });
            queue.push(request);
            Logger::log("Produced " + std::to_string(request) + ". Queue size: " + std::to_string(queue.size()));
        }
        cv.notify_one();
    }

    T consume() {
        T request;
        {
            auto sync = synchronized();
            cv.wait(mutex, [&]() { return not isEmpty(); });
            request = queue.front();
            queue.pop();
            Logger::log("Consumed " + std::to_string(request) + ". Queue size: " + std::to_string(queue.size()));
        }
        cv.notify_one();
        return request;
    }

    [[nodiscard]] bool isFull() const {
        return queue.size() == maxSize;
    }

    [[nodiscard]] bool isEmpty() const {
        return queue.empty();
    }

private:

    std::size_t maxSize;
    DistributedConditionVariable cv;
    std::queue<T> queue;
};

int main(int argc, char** argv) {
    auto communicator = std::make_shared<MpiSimpleCommunicator>(argc, argv);
    Logger::init(communicator);
    Logger::registerThread("Main", rang::fg::cyan);
    auto communicationManager = std::make_shared<CommunicationManager>(communicator);
    auto mutexAlgorithm = std::make_shared<RicartAgrawalaExclusionAlgorithm>(communicationManager);
    auto cvAlgorithm = std::make_shared<DistributedConditionVariableAlgorithm>(communicationManager);

    BufferMonitor<unsigned long> queue("testMon", communicationManager, mutexAlgorithm, cvAlgorithm, MAX_QUEUE_SIZE);
    // We can listen to incoming messages only after constructing the monitor objects because it registers callbacks.
    communicationManager->listen();

    if (communicationManager->getProcessId() % 2 == 0) {
        unsigned long i = 0;
        while (true) {
            queue.produce(i);
            i = (i + 1) % MAX_QUEUE_SIZE;
        }
    } else {
        while (true) {
            queue.consume();
        }
    }
}
