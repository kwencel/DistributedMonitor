# DistributedMonitor
An implementation of a monitor synchronization construct working in the distributed environment.
It uses OpenMPI as a communicator and a Ricart-Agrawala algorithm to perform mutual exclusion.

However, it provieds a very flexible interface so it can work with any kind of communicator and algorithm.
Pull requests are welcome :)

## Build prerequisites
    CMake 3.9 (it will probably compile using older versions too, see the last paragraph)
    C++17 compliant compiler
    OpenMPI
    Boost 1.63.0 (serialization) [optional]

## Build instructions
```
git clone https://github.com/kwencel/DistributedMonitor
cd DistributedMonitor
cmake .
make
```
If you don't have Boost installed, only few example programs will be compiled. If you install Boost later on you will have to repeat the two last instructions for CMake to notice the changes.

## How to use
Invoke compiled executables by mpirun with at least 2 processes, for example:
```
mpirun -np 2 DistributedProdConsSimple
```

## How to implement your own distributed monitor
To create your own monitor you need to take the following steps:
### Extend the DistributedMonitor and create your CV variables
```C++
class BufferMonitor : public DistributedMonitor {
    (...)
    DistributedConditionVariable queueEmptyCv;
    DistributedConditionVariable queueFullCv;
};
```
### Call the base constructor and initialize your CV variables
```C++
BufferMonitor(const std::string& name,
              const std::shared_ptr<CommunicationManager>& communicationManager,
              const std::shared_ptr<IDistributedExclusionAlgorithm>& mutexAlgorithm,
              const std::shared_ptr<IDistributedConditionVariableAlgorithm>& cvAlgorithm)

        : DistributedMonitor(name, communicationManager, mutexAlgorithm),
          queueEmptyCv("empty", cvAlgorithm), queueFullCv("full", cvAlgorithm) { }
```
 
### Provide implementations of pure virtual serialization functions
```C++
std::string saveState() override {
    // Serialize the shared variables to a single string.
}

void restoreState(const std::string_view state) override {
    // Assign the correct values from the string to appropriate shared variables.
}
```
Both functions will be called automatically. You can write them yourself or use a BoostSerializer helper class. See examples for more information.

### Call and store the result of synchronized() function at the begginning of each monitor entry
```C++
void produce() {
    auto sync = synchronized();
    queueFullCv.wait(mutex, [&]() { return queue.size() != MAX_SIZE });
    queue.push(request);
    queueEmptyCv.notify_one();
}

T consume() {
    auto sync = synchronized();
    queueEmptyCv.wait(mutex, [&]() { return not queue.empty() });
    T request = queue.front();
    queue.pop();
    queueFullCv.notify_one();
}
```
It will take care of locking the mutex at the beggining of the entry. It will also send updated state to other processes and release the mutex at the end of the entry.
Do not unlock the mutex yourself. If you want to release the lock before the end of the entry, use an artificial scope and put `sync` object in it.

## Mutex and Condition Variable interface
DistributedMutex and DistributedConditionVariable can be interacted with the similar way you would expect from their counterparts from the Stardard Template Library.
DistributedMutex conforms to the Lockable concept so it can be used with constructs like `std::lock_guard`, `std::unique_lock` or `std::scoped_lock`.
DistributedConditionVariable adapts the concept of a predicate argument from the standard library.
However in STL the predicate argument is optional and you can wrap the condition variable in a `while` loop yourself.
In case of DistributedConditionVariable you cannot do that and you have to provide a `Predicate` instead.

You can use these classes without the need to use DistributedMonitor. They are completely functional standalone. However, you won't be able to synchronize updated states of shared variables between processes.

## Thread safety
At the moment classes from *Distributed* family ane **not** thread-safe. They are meant to be used to procect resources shared by many distributed processes, not threads.
However, thread safety is the next thing I would like to implement in the future.

## Older CMake version?
Try to change the minimum required version in CMakeLists.txt to match the version you have installed. There shouldn't be any issues.
