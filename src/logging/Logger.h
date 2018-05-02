#ifndef DISTRIBUTEDMONITOR_LOGGER_H
#define DISTRIBUTEDMONITOR_LOGGER_H

#include <communication/ICommunicator.h>
#include <memory>
#include <mutex>
#include <thread>
#include <map>
#include "ConsoleColor.h"

class Logger {
public:
    static void init(std::shared_ptr<ICommunicator> communicator);
    static void log(const std::string& message, rang::fg color = rang::fg::reset, rang::style style = rang::style::reset);
    static void registerThread(std::string threadFriendlyName, rang::fg consoleColor = rang::fg::reset);

private:
    static std::string getFormattedNumber(unsigned long number);
    static std::string getCurrentTime();

    static std::mutex mutex;
    static std::map<std::thread::id, std::pair<std::string, rang::fg>> threads;
    static unsigned logMessageCounter;
    static std::shared_ptr<ICommunicator> communicator;
};


#endif //DISTRIBUTEDMONITOR_LOGGER_H
