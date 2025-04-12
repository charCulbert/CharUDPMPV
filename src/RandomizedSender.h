#ifndef RANDOMIZEDSENDER_H
#define RANDOMIZEDSENDER_H

#include <string>
#include <mutex>
#include "UdpComm.h"  // Added to bring in the full definition of UdpComm
#include <random>

class RandomizedSender {
public:
    // Updated constructor with correct parameter types.
    RandomizedSender(const std::string &deviceName, const std::string &deviceIp, UdpComm *udp);

    void setOnOff(bool state);
    void scheduleNext();
    std::string generateRandomCommand();
    int currentSequenceClipsRemaining;
    void sendUdpMessage(const std::string &command);

private:
    std::string deviceName;
    std::string deviceIp;
    UdpComm* udp;
    bool dots_on;
    std::mutex mutex;
    std::mutex genMutex;   // For protecting random engine usage.

    // Each instance has its own random engine.
    std::mt19937 gen;

};

#endif // RANDOMIZEDSENDER_H
