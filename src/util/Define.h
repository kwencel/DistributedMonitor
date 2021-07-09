#ifndef DISTRIBUTEDMONITOR_DEFINE_H
#define DISTRIBUTEDMONITOR_DEFINE_H

#include <map>
#include <functional>
#include "Utils.h"

#define LOGGER_NUMBER_DIGITS 7

using ProcessId = int;
using SubscriptionId = std::size_t;
using LamportTime = unsigned long;
using MutexName = std::string;
using CondName = std::string;
using Predicate = std::function<bool ()>;

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

namespace std {
    template<>
    struct hash<MessageType> {
        inline int operator()(const MessageType& messageType) const {
            return static_cast<std::underlying_type<MessageType>::type>(messageType);
        }
    };
}

#endif //DISTRIBUTEDMONITOR_DEFINE_H
