#ifndef DISTRIBUTEDMONITOR_MPISIMPLECOMMUNICATOR_H
#define DISTRIBUTEDMONITOR_MPISIMPLECOMMUNICATOR_H

#include <mpi.h>
#include <mutex>
#include "ICommunicator.h"

class MpiSimpleCommunicator : public ICommunicator {
public:

    Packet send(MessageType messageType, const std::string& message, ProcessId recipient) override;

    Packet send(MessageType messageType, const std::string& message, const std::unordered_set<ProcessId>& recipients) override;

    Packet sendOthers(MessageType messageType, const std::string& message) override;

    Packet receive() override;

    LamportTime getCurrentLamportTime() override;

    MpiSimpleCommunicator(int argc, char** argv);

    virtual ~MpiSimpleCommunicator();

private:

    static Packet toPacket(RawPacket rawPacket, std::string message);

    MPI_Datatype mpiRawPacketType;
    std::recursive_mutex communicationMutex;
};

#endif //DISTRIBUTEDMONITOR_MPISIMPLECOMMUNICATOR_H
