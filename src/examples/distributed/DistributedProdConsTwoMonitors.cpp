#include <queue>
#include <communication/MpiSimpleCommunicator.h>
#include <distributed/DistributedMonitor.h>
#include <distributed/DistributedConditionVariable.h>
#include "BoostSerializer.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/queue.hpp>
#include <boost/serialization/stack.hpp>

#define MAX_QUEUE_SIZE 5

/**
 * In this example I present two independent monitors (buffers) running in the same program without interfering with
 * each other.
 *
 * Notice that both monitors share the same algorithm objects to act in a distributed fashion.
 * However both monitors could use completely different algorithms and it would work just fine.
 *
 * For example, you could make the first monitor use Ricart-Agrawala algorithm and the second one a Suzuki-Kisami
 * algorithm without any issue. You will just have to provide another implementation of IDistributedExclusionAlgorithm
 */
template <typename T>
class BufferAdv : public DistributedMonitor {
public:

    BufferAdv(const std::string& name,
              const std::shared_ptr<CommunicationManager>& communicationManager,
              const std::shared_ptr<IDistributedExclusionAlgorithm>& mutexAlgorithm,
              const std::shared_ptr<IDistributedConditionVariableAlgorithm>& cvAlgorithm,
              std::size_t queueSize)

            : DistributedMonitor(name, communicationManager, mutexAlgorithm), maxSize(queueSize),
              queueEmptyCv("empty", cvAlgorithm), queueFullCv("full", cvAlgorithm) { };

    std::string saveState() override {
        return BoostSerializer::serialize<std::queue<T>>(queue);
    }

    void restoreState(const std::string_view state) override {
        queue = BoostSerializer::deserialize<std::queue<T>>(state);
    }

    void produce(T request) {
        auto sync = synchronized();
        queueFullCv.wait(mutex, [&]() { return not isFull(); });
        queue.push(request);
        Logger::log("[testMonAdv] Produced " + std::to_string(request) + ". Queue size: " + std::to_string(queue.size()));
        queueEmptyCv.notify_one();
    }

    T consume() {
        auto sync = synchronized();
        queueEmptyCv.wait(mutex, [&]() { return not isEmpty(); });
        T request = queue.front();
        queue.pop();
        Logger::log("[testMonAdv] Consumed " + std::to_string(request) + ". Queue size: " + std::to_string(queue.size()));
        queueFullCv.notify_one();
        return request;
    }

    bool isFull() const {
        return queue.size() == maxSize;
    }

    bool isEmpty() const {
        return queue.empty();
    }

private:

    std::size_t maxSize;
    DistributedConditionVariable queueEmptyCv;
    DistributedConditionVariable queueFullCv;
    std::queue<T> queue;
};

class BufferSimple : public DistributedMonitor {
public:

    BufferSimple(const std::string& name,
                 const std::shared_ptr<CommunicationManager>& communicationManager,
                 const std::shared_ptr<IDistributedExclusionAlgorithm>& mutexAlgorithm,
                 const std::shared_ptr<IDistributedConditionVariableAlgorithm>& cvAlgorithm)

            : DistributedMonitor(name, communicationManager, mutexAlgorithm), cv(name, cvAlgorithm) {
        static_assert(MAX_QUEUE_SIZE <= 10);
    };

    std::string saveState() override {
        std::string state;
        state.resize(3 + MAX_QUEUE_SIZE);
        state[0] = getIndex;
        state[1] = putIndex;
        state[2] = count;
        std::copy(buffer, buffer + MAX_QUEUE_SIZE, state.begin() + 3);
        return state;
    }

    void restoreState(const std::string_view state) override {
        getIndex = state[0];
        putIndex = state[1];
        count = state[2];
        std::copy(state.begin() + 3, state.end(), buffer);
    }

    void produce(char request) {
        auto sync = synchronized();
        cv.wait(mutex, [&]() { return not isFull(); });
        buffer[static_cast<unsigned char>(putIndex)] = request;
        putIndex = (putIndex + 1) % MAX_QUEUE_SIZE;
        ++count;
        Logger::log("[testMonSimple] Produced request " + std::to_string(request) + " at index " + std::to_string(putIndex) + ". Count: " + std::to_string(count));
        cv.notify_one();
    }

    char consume() {
        auto sync = synchronized();
        cv.wait(mutex, [&]() { return not isEmpty(); });
        char request = buffer[static_cast<unsigned char>(getIndex)];
        getIndex = (getIndex + 1) % MAX_QUEUE_SIZE;
        --count;
        Logger::log("[testMonSimple] Consumed request " + std::to_string(request) + " from index " + std::to_string(getIndex) + ". Count: " + std::to_string(count));
        cv.notify_one();
        return request;
    }

    bool isFull() const {
        return count == MAX_QUEUE_SIZE;
    }

    bool isEmpty() const {
        return count == 0;
    }

private:

    char buffer[MAX_QUEUE_SIZE] {0};
    char getIndex = 0;
    char putIndex = 0;
    char count = 0;

    DistributedConditionVariable cv;
};

int main(int argc, char** argv) {
    auto communicator = std::make_shared<MpiSimpleCommunicator>(argc, argv);
    Logger::init(communicator);
    Logger::registerThread("Main", rang::fg::cyan);
    auto communicationManager = std::make_shared<CommunicationManager>(communicator);
    auto mutexAlgorithm = std::make_shared<RicartAgrawalaExclusionAlgorithm>(communicationManager);
    auto cvAlgorithm = std::make_shared<DistributedConditionVariableAlgorithm>(communicationManager);

    BufferAdv<char> queueAdv("testMonAdv", communicationManager, mutexAlgorithm, cvAlgorithm, MAX_QUEUE_SIZE);
    BufferSimple queueSimple("testMonSimple", communicationManager, mutexAlgorithm, cvAlgorithm);
    // We can listen to incoming messages only after constructing the monitor objects because it registers callbacks.
    communicationManager->listen();

    if (communicationManager->getProcessId() % 2 == 0) {
        char i = 0;
        while (true) {
            queueAdv.produce(i);
            queueSimple.produce(i);
            i = (i + 1) % MAX_QUEUE_SIZE;
        }
    } else {
        while (true) {
            queueAdv.consume();
            queueSimple.consume();
        }
    }
}
