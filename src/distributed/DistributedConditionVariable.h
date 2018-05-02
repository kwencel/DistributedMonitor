#ifndef DISTRIBUTEDMONITOR_DISTRIBUTEDCONDITIONVARIABLE_H
#define DISTRIBUTEDMONITOR_DISTRIBUTEDCONDITIONVARIABLE_H

#include <algorithms/IDistributedConditionVariableAlgorithm.h>
#include "DistributedMutex.h"

class DistributedConditionVariable {
public:

    explicit DistributedConditionVariable(std::string name, std::shared_ptr<IDistributedConditionVariableAlgorithm> algorithm)
            : name(std::move(name)), algorithm(std::move(algorithm)) {
        this->algorithm->registerCV(this->name);
    }

    virtual ~DistributedConditionVariable() {
        algorithm->unregisterCV(name);
    }

    void wait(DistributedMutex& mutex, const Predicate& predicate) {
        if (not mutex.isOwned()) {
            throw std::runtime_error("Cannot wait on mutex that is not locked!");
        }
        algorithm->wait(name, mutex, predicate);
    }

    void notify_one() {
        algorithm->notifyOne(name);
    }

    void notify_all() {
        algorithm->notifyAll(name);
    }

    const std::string& getName() const {
        return name;
    }

private:

    std::string name;
    std::shared_ptr<IDistributedConditionVariableAlgorithm> algorithm;
};


#endif //DISTRIBUTEDMONITOR_DISTRIBUTEDCONDITIONVARIABLE_H
