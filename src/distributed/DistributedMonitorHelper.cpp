#include "DistributedMonitor.h"

DistributedMonitorHelper::DistributedMonitorHelper(DistributedMonitor& monitor,
                                                   std::shared_ptr <CommunicationManager> communicationManager)
        : communicationManager(std::move(communicationManager)), monitorMutex(monitor.mutex), monitor(monitor) {

    monitorMutex.lock();
}

DistributedMonitorHelper::~DistributedMonitorHelper() {
    const std::string& serializedState = monitor.saveState();
    communicationManager->sendOthers(MessageType::SYNC, packSyncMessage(serializedState));
    monitorMutex.unlock();
}

std::string DistributedMonitorHelper::packSyncMessage(const std::string& serializedState) {
    const std::string& monitorMutexName = monitor.mutex.getName();
    return static_cast<char>(monitorMutexName.length()) + monitorMutexName + serializedState;
}
