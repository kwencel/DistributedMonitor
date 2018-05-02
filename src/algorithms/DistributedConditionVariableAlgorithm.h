#ifndef DISTRIBUTEDMONITOR_DISTRIBUTEDCONDITIONVARIABLEALGORITHM_H
#define DISTRIBUTEDMONITOR_DISTRIBUTEDCONDITIONVARIABLEALGORITHM_H

#include <condition_variable>
#include <communication/CommunicationManager.h>
#include <util/Utils.h>
#include <set>
#include <logging/ConsoleColor.h>
#include <logging/Logger.h>
#include "IDistributedConditionVariableAlgorithm.h"

class DistributedConditionVariableAlgorithm : public IDistributedConditionVariableAlgorithm {
public:

    explicit DistributedConditionVariableAlgorithm(std::shared_ptr<CommunicationManager> communicationManager)
            : communicationManager(std::move(communicationManager)) {
        /** Conditional variables waits handling **/
        this->communicationManager->subscribe(
            [&](Packet packet) {
                const CondName& condName = packet.message;
                std::lock_guard<std::mutex> guard(registerCVsMutex);
                return packet.messageType == MessageType::COND_WAIT and contains(registeredCVs, condName);
            },
            [&](Packet waitInfo) {
                const CondName& condName = waitInfo.message;
                std::lock_guard<std::mutex> guard(conditionalWaitsMutex);
                conditionalWaits[condName].insert(waitInfo);
            }
        );
        /** Conditional variables waits ends handling **/
        this->communicationManager->subscribe(
            [&](Packet packet) {
                const CondName& condName = packet.message;
                std::lock_guard<std::mutex> guard(registerCVsMutex);
                return packet.messageType == MessageType::COND_WAIT_END and contains(registeredCVs, condName);
            },
            [&](Packet waitEndInfo) {
                const CondName& condName = waitEndInfo.message;
                {
                    std::lock_guard<std::mutex> guard(conditionalWaitsMutex);
                    conditionalWaits.erase(condName);
                }
                this->communicationManager->send(MessageType::COND_WAIT_END_CONFIRM, condName, waitEndInfo.source);
            }
        );
    }

    void registerCV(const CondName& condName) override {
        std::lock_guard<std::mutex> guard(registerCVsMutex);
        registeredCVs.insert(condName);
    }

    void unregisterCV(const CondName& condName) override {
        std::lock_guard<std::mutex> guard(registerCVsMutex);
        registeredCVs.erase(condName);
    }

    void wait(const CondName& condName, DistributedMutex& mutex, const Predicate& predicate) override {
        std::mutex allConfirmationsMutex;
        std::condition_variable allConfirmationsCondition;
        std::condition_variable_any realCond;
        std::unordered_set<Packet> receivedConfirmations;

        /** Conditional variable notify handling **/
        SubscriptionId notifySubscriptionId = communicationManager->subscribe(
                /** Predicate function - Accessed by Receiving Thread **/
                [&](Packet packet) {
                    return packet.messageType == MessageType::COND_NOTIFY and packet.message == condName;
                },
                /** Callback function - Accessed by Receiving Thread **/
                [&](Packet notification) {
                    realCond.notify_one();
                }
        );

        communicationManager->sendOthers(MessageType::COND_WAIT, condName);
        Logger::log("Started waiting on CV '" + condName + "'", rang::fg::cyan);
        realCond.wait(mutex, predicate);
        Logger::log("Stopped waiting on CV '" + condName + "'", rang::fg::magenta);

        SubscriptionId confirmationsSubscriptionId = communicationManager->subscribe(
                /** Predicate function - Accessed by Receiving Thread **/
                [&](Packet packet) {
                    return packet.messageType == MessageType::COND_WAIT_END_CONFIRM and packet.message == condName;
                },
                /** Callback function - Accessed by Receiving Thread **/
                [&](Packet confirmation) {
                    std::unique_lock<std::mutex> guard(allConfirmationsMutex);
                    receivedConfirmations.insert(confirmation);
                    if (areConfirmationsComplete(receivedConfirmations)) {
                        guard.unlock();
                        allConfirmationsCondition.notify_one();
                    }
                }
        );

        communicationManager->sendOthers(MessageType::COND_WAIT_END, condName);
        /** Wait until all processed confirm that they received our COND_WAIT_END message **/
        std::unique_lock<std::mutex> allConfirmationsLock(allConfirmationsMutex);
        allConfirmationsCondition.wait(allConfirmationsLock, [&]() { return areConfirmationsComplete(receivedConfirmations); });
        communicationManager->unsubscribe(notifySubscriptionId);
        communicationManager->unsubscribe(confirmationsSubscriptionId);
    }

    void notifyOne(const CondName& condName) override {
        std::unique_lock<std::mutex> guard(conditionalWaitsMutex);
        if (conditionalWaits[condName].empty()) {
            Logger::log("There is no one to notify");
            return;
        }
        ProcessId firstWaitingProcess = (*(conditionalWaits[condName].begin())).source;
        guard.unlock();
        communicationManager->send(MessageType::COND_NOTIFY, condName, firstWaitingProcess);
    }

    void notifyAll(const CondName& condName) override {
        std::unique_lock<std::mutex> guard(conditionalWaitsMutex);
        if (conditionalWaits.empty()) {
            Logger::log("There is no one to notify");
            return;
        }
        std::unordered_set<ProcessId> recipients;
        const std::set<Packet>& waiting = conditionalWaits[condName];
        std::transform(waiting.begin(), waiting.end(), std::inserter(recipients, recipients.begin()), [&](Packet packet) {
            return packet.source;
        });
        guard.unlock();
        communicationManager->send(MessageType::COND_NOTIFY, condName, recipients);
    }

    ProcessId getProcessId() override {
        return communicationManager->getProcessId();
    }

private:

    bool areConfirmationsComplete(const std::unordered_set<Packet>& receivedConfirmations) const {
        return receivedConfirmations.size() == static_cast<std::size_t>(communicationManager->getNumberOfProcesses()) - 1;
    }

    std::unordered_set<CondName> registeredCVs;
    std::map<CondName, std::set<Packet>> conditionalWaits;
    std::shared_ptr<CommunicationManager> communicationManager;

    std::mutex conditionalWaitsMutex;
    std::mutex registerCVsMutex;
};

#endif //DISTRIBUTEDMONITOR_DISTRIBUTEDCONDITIONVARIABLEALGORITHM_H
