cmake_minimum_required(VERSION 3.9)
project(DistributedMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.openmpi")

find_package(MPI REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost COMPONENTS serialization)
include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})

file(GLOB SOURCE_FILES "src/algorithms/*" "src/communication/*" "src/distributed/*" "src/logging/*" "src/util/*")
include_directories(src)

# Local examples for comparison with distributed ones. No distributed algorithms nor interprocess communication.
# They only use synchronization mechanismsavailable in C++ standard library
add_executable(LocalProdConsSingleCV src/examples/local/LocalProdConsSingleCV.cpp)
target_link_libraries(LocalProdConsSingleCV ${CMAKE_THREAD_LIBS_INIT})

add_executable(LocalProdConsTwoCV src/examples/local/LocalProdConsTwoCV.cpp)
target_link_libraries(LocalProdConsTwoCV ${CMAKE_THREAD_LIBS_INIT})

add_executable(LocalProdConsTwoCVMulti src/examples/local/LocalProdConsTwoCVMulti.cpp)
target_link_libraries(LocalProdConsTwoCVMulti ${CMAKE_THREAD_LIBS_INIT})

# Distributed examples, using distributed monitors
add_executable(DistributedProdConsSimple ${SOURCE_FILES} src/examples/distributed/DistributedProdConsSimple.cpp)
target_link_libraries(DistributedProdConsSimple ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(DistributedProdConsSimpleTwoCV ${SOURCE_FILES} src/examples/distributed/DistributedProdConsSimpleTwoCV.cpp)
target_link_libraries(DistributedProdConsSimpleTwoCV ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if (Boost_FOUND)

    add_executable(DistributedProdConsSingleCV ${SOURCE_FILES} src/examples/distributed/BoostSerializer.h src/examples/distributed/DistributedProdConsSingleCV.cpp)
    target_link_libraries(DistributedProdConsSingleCV ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

    add_executable(DistributedProdConsSingleCVUnlockFirst ${SOURCE_FILES} src/examples/distributed/BoostSerializer.h src/examples/distributed/DistributedProdConsSingleCVUnlockFirst.cpp)
    target_link_libraries(DistributedProdConsSingleCVUnlockFirst ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

    add_executable(DistributedProdConsTwoCV ${SOURCE_FILES} src/examples/distributed/BoostSerializer.h src/examples/distributed/DistributedProdConsTwoCV.cpp)
    target_link_libraries(DistributedProdConsTwoCV ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

    add_executable(DistributedProdConsTwoMonitors ${SOURCE_FILES} src/examples/distributed/BoostSerializer.h src/examples/distributed/DistributedProdConsTwoMonitors.cpp)
    target_link_libraries(DistributedProdConsTwoMonitors ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

endif()