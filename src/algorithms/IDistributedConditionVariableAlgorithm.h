#ifndef DISTRIBUTEDMONITOR_IDISTRIBUTEDCONDITIONVARIABLEALGORITHM_H
#define DISTRIBUTEDMONITOR_IDISTRIBUTEDCONDITIONVARIABLEALGORITHM_H

#include <util/define.h>
#include <distributed/DistributedMutex.h>

class IDistributedConditionVariableAlgorithm {
public:

    virtual void registerCV(const CondName& condName) = 0;

    virtual void unregisterCV(const CondName& condName) = 0;

    virtual void wait(const CondName& condName, DistributedMutex& mutex, const Predicate& predicate) = 0;

    virtual void notifyOne(const CondName& condName) = 0;

    virtual void notifyAll(const CondName& condName) = 0;

    virtual ProcessId getProcessId() = 0;

};

#endif //DISTRIBUTEDMONITOR_IDISTRIBUTEDCONDITIONVARIABLEALGORITHM_H
