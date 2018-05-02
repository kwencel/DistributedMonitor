#ifndef DISTRIBUTEDMONITOR_UTILS_H
#define DISTRIBUTEDMONITOR_UTILS_H

#include <string>
#include <array>
#include <memory>

inline std::string execAndCaptureStdOut(const char* command) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(command, "r"), pclose);
    if (not pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (not feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

template<class Container, class T>
auto contains(const Container& container, const T& x) -> decltype(container.find(x) != container.end()) {
    return container.find(x) != container.end();
}

template <typename Container>
inline std::string printContainer(const Container& container) {
    if (container.empty()) {
        return "{}";
    }
    std::string result = "{" + std::to_string(*(container.begin()));
    if (container.size() == 1) {
        return result + "}";
    }
    for (auto it = std::next(container.begin()); it != container.end(); ++it) {
        result += "," + std::to_string(*it);
    }
    result += '}';
    return result;
}


#endif //DISTRIBUTEDMONITOR_UTILS_H
