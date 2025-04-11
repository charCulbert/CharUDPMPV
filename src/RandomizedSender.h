#ifndef RANDOMIZEDSENDER_H
#define RANDOMIZEDSENDER_H

#include <string>
#include <mutex>
#include "UdpComm.h"  // Added to bring in the full definition of UdpComm

class RandomizedSender {
public:
    // Updated constructor with correct parameter types.
    RandomizedSender(const std::string &deviceName, const std::string &deviceIp, UdpComm *udp);

    void setOnOff(bool state);
    void scheduleNext();
    std::string generateRandomCommand();
    void sendUdpMessage(const std::string &command);

private:
    std::string deviceName;
    std::string deviceIp;
    UdpComm* udp;
    bool dots_on;
    std::mutex mutex;
};

#endif // RANDOMIZEDSENDER_H
