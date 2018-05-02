#ifndef DISTRIBUTEDMONITOR_DISTRIBUTEDMONITORHELPER_H
#define DISTRIBUTEDMONITOR_DISTRIBUTEDMONITORHELPER_H

#include <communication/CommunicationManager.h>
#include "DistributedMutex.h"

class DistributedMonitor;

class DistributedMonitorHelper {
public:

    /**
     * Locks the mutex
     */
    explicit DistributedMonitorHelper(DistributedMonitor& monitor, std::shared_ptr<CommunicationManager> communicationManager);

    /**
     * Sends synchronization message and unlocks the mutex afterwards
     */
    virtual ~DistributedMonitorHelper();

private:

    std::string packSyncMessage(const std::string& serializedState);

    std::shared_ptr<CommunicationManager> communicationManager;
    DistributedMutex& monitorMutex;
    DistributedMonitor& monitor;
};

#endif //DISTRIBUTEDMONITOR_DISTRIBUTEDMONITORHELPER_H
