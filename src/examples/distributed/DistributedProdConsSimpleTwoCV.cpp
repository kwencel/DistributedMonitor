#include <distributed/DistributedMonitor.h>
#include <distributed/DistributedConditionVariable.h>
#include <communication/MpiSimpleCommunicator.h>

#define MAX_QUEUE_SIZE 5

/**
 * In this example I present a Producer-Consumer problem without the Boost dependency using two Condition Variables.
 *
 * Serialization is written by hand because the underlying buffer structure is just a char array with an assumption that
 * each element of that array is a one single digit.
 */
class BufferMonitor : public DistributedMonitor {
public:

    BufferMonitor(const std::string& name,
                  const std::shared_ptr<CommunicationManager>& communicationManager,
                  const std::shared_ptr<IDistributedExclusionAlgorithm>& mutexAlgorithm,
                  const std::shared_ptr<IDistributedConditionVariableAlgorithm>& cvAlgorithm)

            : DistributedMonitor(name, communicationManager, mutexAlgorithm), queueEmptyCv("empty", cvAlgorithm),
              queueFullCv("full", cvAlgorithm) {

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
        queueFullCv.wait(mutex, [&]() { return not isFull(); });
        buffer[static_cast<unsigned char>(putIndex)] = request;
        ++count;
        Logger::log("Produced request " + std::to_string(request) + " at index " + std::to_string(putIndex) + ". Count: " + std::to_string(count));
        putIndex = (putIndex + 1) % MAX_QUEUE_SIZE;
        queueEmptyCv.notify_one();
    }

    char consume() {
        auto sync = synchronized();
        queueEmptyCv.wait(mutex, [&]() { return not isEmpty(); });
        char request = buffer[static_cast<unsigned char>(getIndex)];
        --count;
        Logger::log("Consumed request " + std::to_string(request) + " from index " + std::to_string(getIndex) + ". Count: " + std::to_string(count));
        getIndex = (getIndex + 1) % MAX_QUEUE_SIZE;
        queueFullCv.notify_one();
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

    DistributedConditionVariable queueEmptyCv;
    DistributedConditionVariable queueFullCv;
};

int main(int argc, char** argv) {
    auto communicator = std::make_shared<MpiSimpleCommunicator>(argc, argv);
    Logger::init(communicator);
    Logger::registerThread("Main", rang::fg::cyan);
    auto communicationManager = std::make_shared<CommunicationManager>(communicator);
    auto mutexAlgorithm = std::make_shared<RicartAgrawalaExclusionAlgorithm>(communicationManager);
    auto cvAlgorithm = std::make_shared<DistributedConditionVariableAlgorithm>(communicationManager);

    BufferMonitor queue("testMon", communicationManager, mutexAlgorithm, cvAlgorithm);
    // We can listen to incoming messages only after constructing the monitor objects because it registers callbacks.
    communicationManager->listen();

    if (communicationManager->getProcessId() % 2 == 0) {
        char i = 0;
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
