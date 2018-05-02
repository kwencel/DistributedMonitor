#include <mutex>
#include <memory>
#include "MpiSimpleCommunicator.h"

Packet MpiSimpleCommunicator::send(MessageType messageType, const std::string& message,
                                   const std::unordered_set<ProcessId>& recipients) {

    std::lock_guard<std::recursive_mutex> lock(communicationMutex);

    RawPacket rawPacket {
            .lamportTime = static_cast<uint64_t> (++currentLamportTime),
            .source = static_cast<int32_t>(myProcessId),
            .messageType = static_cast<uint8_t> (messageType),
            .nextPacketLength = static_cast<uint32_t>(message.size()),
    };

    for (ProcessId recipient : recipients) {
        MPI_Send(&rawPacket, 1, mpiRawPacketType, recipient, 0, MPI_COMM_WORLD);
        if (not message.empty()) {
            MPI_Send(message.c_str(), static_cast<int>(message.size()), MPI_CHAR, recipient, 0, MPI_COMM_WORLD);
        }
    }

    Packet packet {
            .lamportTime = rawPacket.lamportTime,
            .source = rawPacket.source,
            .messageType = messageType,
            .message = message
    };

    return packet;
}

Packet MpiSimpleCommunicator::sendOthers(MessageType messageType, const std::string& message) {
    return send(messageType, message, otherProcesses);
}

Packet MpiSimpleCommunicator::send(MessageType messageType, const std::string& message, ProcessId recipient) {
    return send(messageType, message, std::unordered_set<ProcessId> {recipient});
}

Packet MpiSimpleCommunicator::receive() {
    MPI_Status status;
    RawPacket rawPacket;
    MPI_Recv(&rawPacket, 1, mpiRawPacketType, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    uint32_t messageLength = rawPacket.nextPacketLength;
    std::string message;
    message.resize(messageLength);
    MPI_Recv(message.data(), messageLength, MPI_CHAR, rawPacket.source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    {
        std::lock_guard<std::recursive_mutex> lock(communicationMutex);
        currentLamportTime = std::max(rawPacket.lamportTime, currentLamportTime) + 1;
    }
    return toPacket(rawPacket, message);
}

MpiSimpleCommunicator::MpiSimpleCommunicator(int argc, char** argv) {
    int provided = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    /*************** Create a type for a custom 'RawPacket' structure ***************/
    const int fields = 4;
    int blockLengths[fields] = {1, 1, 1, 1};
    MPI_Datatype types[fields] = {MPI_UINT64_T, MPI_INT32_T, MPI_UINT8_T, MPI_UINT32_T};
    MPI_Aint offsets[fields];

    offsets[0] = offsetof(RawPacket, lamportTime);
    offsets[1] = offsetof(RawPacket, source);
    offsets[2] = offsetof(RawPacket, messageType);
    offsets[3] = offsetof(RawPacket, nextPacketLength);

    MPI_Type_create_struct(fields, blockLengths, offsets, types, &mpiRawPacketType);
    MPI_Type_commit(&mpiRawPacketType);
    /*****************************************************************************/

    MPI_Comm_rank(MPI_COMM_WORLD, &myProcessId);
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcesses);

    /************ Fill 'otherProcesses' structure with their ranks ************/
    for (ProcessId id = 0; id < numberOfProcesses; ++id) {
        if (id != myProcessId) {
            otherProcesses.insert(id);
        }
    }
    /**************************************************************************/

    currentLamportTime = 0;
}

Packet MpiSimpleCommunicator::toPacket(RawPacket rawPacket, std::string message) {
    return Packet {
            .lamportTime = static_cast<LamportTime>(rawPacket.lamportTime),
            .source = static_cast<ProcessId>(rawPacket.source),
            .messageType = static_cast<MessageType>(rawPacket.messageType),
            .message = std::move(message)
    };
}

LamportTime MpiSimpleCommunicator::getCurrentLamportTime() {
    std::lock_guard<std::recursive_mutex> lock(communicationMutex);
    return currentLamportTime;
}

MpiSimpleCommunicator::~MpiSimpleCommunicator() {
    MPI_Finalize();
}
