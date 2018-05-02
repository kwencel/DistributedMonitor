#ifndef DISTRIBUTEDMONITOR_MONITOR_H
#define DISTRIBUTEDMONITOR_MONITOR_H

#include <cassert>
#include <utility>
#include <communication/CommunicationManager.h>
#include <algorithms/IDistributedExclusionAlgorithm.h>
#include <algorithms/IDistributedConditionVariableAlgorithm.h>
#include <algorithms/RicartAgrawalaExclusionAlgorithm.h>
#include <algorithms/DistributedConditionVariableAlgorithm.h>
#include "DistributedMonitorHelper.h"

class DistributedMonitor  {
    friend DistributedMonitorHelper;

public:

    explicit DistributedMonitor(const std::string& name,
                                std::shared_ptr<CommunicationManager> communicationManager,
                                const std::shared_ptr<IDistributedExclusionAlgorithm>& mutexAlgorithm)

            : communicationManager(std::move(communicationManager)), mutex(name, mutexAlgorithm) {

        assert(name.length() <= static_cast<unsigned char>(-1));

        /** Restore the state based on the SYNC message **/
        syncSubscription = this->communicationManager->subscribe(
                [&](Packet packet) {
                    return packet.messageType == MessageType::SYNC and extractMutexName(packet.message) == mutex.getName();
                },
                [&](Packet data) {
                    return restoreState(extractSerializedData(data.message));
                }
        );
    };

    virtual ~DistributedMonitor() {
        communicationManager->unsubscribe(syncSubscription);
    }

private:

    static const std::string_view extractMutexName(const std::string& message) {
        return std::string_view(message).substr(1, static_cast<std::string_view::size_type>(message[0]));
    }

    static std::string extractSerializedData(const std::string& message) {
        auto dataOffset = static_cast<std::string_view::size_type>(message[0]) + 1;
        return message.substr(dataOffset);
    }

    std::shared_ptr<CommunicationManager> communicationManager;
    SubscriptionId syncSubscription;

protected:

    /** Will be called automatically after any monitor entry function. */
    virtual std::string saveState() = 0;

    /** Will be called automatically before each monitor entry function. */
    virtual void restoreState(std::string_view) = 0;

    /** Invoke this method at the beginning of the derived monitor class' entries */
    std::unique_ptr<DistributedMonitorHelper> synchronized() {
        return std::make_unique<DistributedMonitorHelper>(*this, communicationManager);
    }

    DistributedMutex mutex;
};

#endif //DISTRIBUTEDMONITOR_MONITOR_H
