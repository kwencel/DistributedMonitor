#ifndef DISTRIBUTEDMONITOR_BOOSTSERIALIZER_H
#define DISTRIBUTEDMONITOR_BOOSTSERIALIZER_H

#include <sstream>
#include <string>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace BoostSerializer {

    template <typename T>
    static std::string serialize(T object) {
        std::ostringstream archiveStream;
        {
            boost::archive::text_oarchive archive(archiveStream);
            archive << object;
        }
        return archiveStream.str();
    }

    template <typename T>
    static T deserialize(const std::string_view serializedObject) {
        T deserializedObject;
        std::stringstream archiveStream;
        archiveStream << serializedObject;
        {
            boost::archive::text_iarchive archive(archiveStream);
            archive >> deserializedObject;
        }
        return deserializedObject;
    }
}

#endif //DISTRIBUTEDMONITOR_BOOSTSERIALIZER_H
