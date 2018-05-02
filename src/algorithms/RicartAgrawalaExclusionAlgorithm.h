#ifndef DISTRIBUTEDMONITOR_RICARTAGRAWALAEXCLUSIONALGORITHM_H
#define DISTRIBUTEDMONITOR_RICARTAGRAWALAEXCLUSIONALGORITHM_H

#include <communication/CommunicationManager.h>
#include <util/Utils.h>
#include <condition_variable>
#include <logging/Logger.h>
#include <unordered_set>
#include <sstream>
#include "IDistributedExclusionAlgorithm.h"

class RicartAgrawalaExclusionAlgorithm : public IDistributedExclusionAlgorithm {
public:

    explicit RicartAgrawalaExclusionAlgorithm(std::shared_ptr<CommunicationManager> communicationManager)
            : communicationManager(std::move(communicationManager)) {
        /** Mutex requests handling **/
        this->communicationManager->subscribe(
                [&](Packet packet) {
                    const MutexName& mutexName = packet.message;
                    std::lock_guard<std::mutex> guard(registeredMutexesMutex);
                    return packet.messageType == MessageType::MUTEX_REQUEST and contains(registeredMutexes, mutexName);
                },
                [&](Packet request) {
                    processRequest(request);
                }
        );
    }

    void registerMutex(const MutexName& mutexName) override {
        std::lock_guard<std::mutex> guard(registeredMutexesMutex);
        registeredMutexes.insert(mutexName);
    }

    void unregisterMutex(const MutexName& mutexName) override {
        std::lock_guard<std::mutex> guard(registeredMutexesMutex);
        registeredMutexes.erase(mutexName);
    }

    /** Accessed by Main Thread **/
    void acquireMutex(const MutexName& mutexName) override {
        std::mutex allAgreementsMutex;
        std::condition_variable allAgreementsCondition;
        std::unordered_set<Packet> receivedAgreements;

        SubscriptionId subscriptionId = communicationManager->subscribe(
                /** Predicate function - Accessed by Receiving Thread **/
                [&](Packet packet) {
                    return packet.messageType == MessageType::MUTEX_AGREEMENT and packet.message == mutexName;
                },
                /** Callback function - Accessed by Receiving Thread **/
                [&](Packet agreement) {
                    std::unique_lock<std::mutex> guard(allAgreementsMutex);
                    addAgreement(receivedAgreements, agreement);
                    auto remainingAgreements = communicationManager->getNumberOfProcesses() - 1 - receivedAgreements.size();
                    Logger::log("Agreements remaining: " + std::to_string(remainingAgreements));
                    if (areAgreementsComplete(receivedAgreements)) {
                        guard.unlock();
                        allAgreementsCondition.notify_one();
                    }
                }
        );

        queue(mutexName);
        std::unique_lock<std::mutex> allAgreementsLock(allAgreementsMutex);
        allAgreementsCondition.wait(allAgreementsLock, [&]() { return areAgreementsComplete(receivedAgreements); });
        communicationManager->unsubscribe(subscriptionId);
        Logger::log("Mutex '" + mutexName + "' acquired", rang::fg::blue);
        // Mutex acquired - can enter critical section
    }

    /** Accessed by Main Thread **/
    void releaseMutex(const MutexName& mutexName) override {
        std::scoped_lock lock(queuedMutexesMutex, receivedRequestsMutex);
        Logger::log("Mutex '" + mutexName + "' released", rang::fg::yellow);
        for (const Packet& request : receivedRequests[mutexName]) {
            sendAgreement(request);
        }

        receivedRequests[mutexName].clear();
        queuedMutexes.erase(mutexName);
    }

    ProcessId getProcessId() override {
        return communicationManager->getProcessId();
    }

private:

    /** Accessed by Receiving Thread **/
    bool addAgreement(std::unordered_set<Packet>& receivedAgreements, const Packet& agreement) {
        const std::string& mutexName = agreement.message;
        std::lock_guard<std::mutex> lock(queuedMutexesMutex);
        bool amQueuedInMutex = contains(queuedMutexes, mutexName);
        if (amQueuedInMutex) {
            /** If I am queued in this mutex - accept the agreement **/
            receivedAgreements.insert(agreement);
            return true;
        } else {
            /** I did not queue in this mutex. It should not happen. **/
            Logger::log("Received agreement from Process " + std::to_string(agreement.source) + " concerning mutex " +
                        mutexName + " which I did not intend to acquire");
            throw std::runtime_error("Received agreement on acquiring mutex I was not interested in acquiring");
        }
    }

    /** Accessed by Main Thread */
    void queue(const MutexName& mutexName) {
        std::lock_guard<std::mutex> lock(queuedMutexesMutex);
        Packet packet = communicationManager->sendOthers(MessageType::MUTEX_REQUEST, mutexName);
        queuedMutexes[mutexName] = packet.lamportTime;
    }

    /** Accessed by Receiving Thread */
    void processRequest(const Packet& request) {
        const MutexName& mutexName = request.message;
        std::scoped_lock lock(queuedMutexesMutex, receivedRequestsMutex);
        if (canSendAgreement(request)) {
            sendAgreement(request);
        } else {
            receivedRequests[mutexName].insert(request);
        }
    }

    /** Accessed by Receiving Thread */
    bool canSendAgreement(const Packet& request) {
        std::stringstream acceptanceLoggerMessage;
        const MutexName& mutexName = request.message;
        acceptanceLoggerMessage << "Allowing process " << std::to_string(request.source) << " to acquire mutex " <<
                                   request.message << " because ";

        auto queuedMutexPtr = queuedMutexes.find(mutexName);
        auto queuedMutexEnd = queuedMutexes.end();
        /** I'm not interested in acquiring the mutex **/
        if (queuedMutexPtr == queuedMutexEnd) {
            acceptanceLoggerMessage << "I am not interested";
            Logger::log(acceptanceLoggerMessage.str());
            return true;
        }

        /** Request's logical clock is lower than my logical clock' **/
        LamportTime myRequestLamportTime = (*queuedMutexPtr).second;
        if (request.lamportTime < myRequestLamportTime) {
            acceptanceLoggerMessage << "incoming request has lower TS " <<
                                    "(" << request.lamportTime << " vs " << myRequestLamportTime << ")";
            Logger::log(acceptanceLoggerMessage.str());
            return true;
        }

        /** Request's logical clock is the same as the my logical clock and I have the higher process ID **/
        if ((request.lamportTime == myRequestLamportTime) and (request.source < communicationManager->getProcessId())) {
            acceptanceLoggerMessage << "sender has lower ID";
            Logger::log(acceptanceLoggerMessage.str());
            return true;
        }

        Logger::log("Delaying the agreement to process " + std::to_string(request.source));
        return false;
    }

    /** Accessed by Main and Receiving threads **/
    void sendAgreement(const Packet& request) {
        ProcessId processId = request.source;
        const MutexName& mutexName = request.message;
        communicationManager->send(MessageType::MUTEX_AGREEMENT, mutexName, processId);
    }

    /** Accessed by Main and Receiving thread - protected by allAgreementsMutex **/
    bool areAgreementsComplete(const std::unordered_set<Packet>& receivedAgreements) {
        return receivedAgreements.size() == static_cast<std::size_t>(communicationManager->getNumberOfProcesses()) - 1;
    }

    std::unordered_set<MutexName> registeredMutexes;
    std::map<MutexName, LamportTime> queuedMutexes;
    std::map<MutexName, std::unordered_set<Packet>> receivedRequests;

    std::mutex registeredMutexesMutex;
    std::mutex receivedRequestsMutex;
    std::mutex queuedMutexesMutex;

    std::shared_ptr<CommunicationManager> communicationManager;

};

#endif //DISTRIBUTEDMONITOR_RICARTAGRAWALAEXCLUSIONALGORITHM_H
