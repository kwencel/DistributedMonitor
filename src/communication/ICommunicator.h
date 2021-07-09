#ifndef DISTRIBUTEDMONITOR_ICOMMUNICATOR_H
#define DISTRIBUTEDMONITOR_ICOMMUNICATOR_H

#include <unordered_set>
#include <util/Define.h>

class ICommunicator {
public:

    virtual Packet send(MessageType messageType, const std::string& message, ProcessId recipient) = 0;

    virtual Packet send(MessageType messageType, const std::string& message, const std::unordered_set<ProcessId>& recipients) = 0;

    virtual Packet sendOthers(MessageType messageType, const std::string& message) = 0;

    virtual Packet receive() = 0;

    virtual ProcessId getProcessId() {
        return myProcessId;
    }

    virtual ProcessId getNumberOfProcesses() {
        return numberOfProcesses;
    }

    virtual LamportTime getCurrentLamportTime() {
        return currentLamportTime;
    }

protected:

    ProcessId myProcessId;

    ProcessId numberOfProcesses;

    std::unordered_set<ProcessId> otherProcesses;

    LamportTime currentLamportTime;
};

#endif //DISTRIBUTEDMONITOR_ICOMMUNICATOR_H
