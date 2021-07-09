#ifndef DISTRIBUTEDMONITOR_IDISTRIBUTEDEXCLUSIONALGORITHM_H
#define DISTRIBUTEDMONITOR_IDISTRIBUTEDEXCLUSIONALGORITHM_H

#include <util/Define.h>

class IDistributedExclusionAlgorithm {
public:

    /** Associates a mutex with the given algorithm. It is crucial to know which algorithms handle which mutexes. */
    virtual void registerMutex(const MutexName& mutexName) = 0;

    virtual void unregisterMutex(const MutexName& mutexName) = 0;

    /** This function blocks until all other remote processes agree for this process to acquire the mutex */
    virtual void acquireMutex(const MutexName& mutexName) = 0;

    virtual void releaseMutex(const MutexName& mutexName) = 0;

    virtual ProcessId getProcessId() = 0;
};

#endif //DISTRIBUTEDMONITOR_IDISTRIBUTEDEXCLUSIONALGORITHM_H
