#ifndef DISTRIBUTEDMONITOR_DISTRIBUTEDMUTEX_H
#define DISTRIBUTEDMONITOR_DISTRIBUTEDMUTEX_H

#include <atomic>
#include <memory>

/**
 * Assumption: There will be only one shared mutex associated with the monitor!
 *
 * A DistributedMutex implementing the Lockable concept in C++.
 */
class DistributedMutex {
public:

    explicit DistributedMutex(MutexName name, std::shared_ptr<IDistributedExclusionAlgorithm> algorithm)
            : name(std::move(name)), algorithm(std::move(algorithm)) {
        this->algorithm->registerMutex(this->name);
    }

    virtual ~DistributedMutex() {
        algorithm->unregisterMutex(name);
    }

    void lock() {
        algorithm->acquireMutex(name);
        owned = true;
    }

    void unlock() {
        if (not isOwned()) {
            return;
        }
        algorithm->releaseMutex(name);
        owned = false;
    }

    bool try_lock() {
        if (not isOwned()) {
            lock();
            return true;
        }
        return false;
    }

    bool isOwned() {
        return owned.load();
    }

    [[nodiscard]] const std::string& getName() const {
        return name;
    }

private:

    std::string name;
    std::shared_ptr<IDistributedExclusionAlgorithm> algorithm;
    std::atomic_bool owned = false;
};


#endif //DISTRIBUTEDMONITOR_DISTRIBUTEDMUTEX_H
