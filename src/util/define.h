#ifndef DISTRIBUTEDMONITOR_DEFINE_H
#define DISTRIBUTEDMONITOR_DEFINE_H

#include <map>
#include <functional>

#define LOGGER_NUMBER_DIGITS 7

using ProcessId = int;
using SubscriptionId = std::size_t;
using LamportTime = unsigned long;
using MutexName = std::string;
using CondName = std::string;

enum class MessageType : unsigned char {
    MUTEX_REQUEST, MUTEX_AGREEMENT, COND_WAIT, COND_WAIT_END, COND_WAIT_END_CONFIRM, COND_NOTIFY, SYNC
};

static std::map<MessageType, std::string>  messageTypeString = {{MessageType::MUTEX_REQUEST, "MUTEX_REQUEST"},
                                                                {MessageType::MUTEX_AGREEMENT, "MUTEX_AGREEMENT"},
                                                                {MessageType::COND_WAIT, "COND_WAIT"},
                                                                {MessageType::COND_WAIT_END, "COND_WAIT_END"},
                                                                {MessageType::COND_WAIT_END_CONFIRM, "COND_WAIT_END_CONFIRM"},
                                                                {MessageType::COND_NOTIFY, "COND_NOTIFY"},
                                                                {MessageType::SYNC, "SYNC"}};

inline std::ostream& operator<< (std::ostream& os, MessageType messageType) {
    return os << messageTypeString[messageType];
}

inline std::string& operator+ (std::string& str, MessageType messageType) {
    return str.append(messageTypeString[messageType]);
}

struct Packet {
    LamportTime lamportTime;
    ProcessId source;
    MessageType messageType;
    std::string message;

    inline bool operator == (const Packet& other) const {
        return source == other.source && messageType == other.messageType && message == other.message;
    }

    inline bool operator < (const Packet& other) const {
        return lamportTime < other.lamportTime;
    }
};

struct RawPacket {
    uint64_t lamportTime;
    int32_t source;
    uint8_t messageType;
    uint32_t nextPacketLength;
};

using Predicate = std::function<bool ()>;
using SubscriptionPredicate = std::function<bool (const Packet&)>;
using SubscriptionCallback = std::function<void (const Packet&)>;

inline void hash_combine(std::size_t& seed) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine(seed, rest...);
}

namespace std {
    template<>
    struct hash<Packet> {
        inline std::size_t operator()(const Packet& packet) const {
            std::size_t hash = 0;
            hash_combine(hash, packet.source, packet.messageType, packet.message);
            return hash;
        }
    };
    template<>
    struct hash<RawPacket> {
        inline std::size_t operator()(const RawPacket& packet) const {
            std::size_t hash = 0;
            hash_combine(hash, packet.source, packet.messageType, packet.nextPacketLength);
            return hash;
        }
    };
    template<>
    struct hash<MessageType> {
        inline int operator()(const MessageType& messageType) const {
            return static_cast<std::underlying_type<MessageType>::type>(messageType);
        }
    };
}

#endif //DISTRIBUTEDMONITOR_DEFINE_H
