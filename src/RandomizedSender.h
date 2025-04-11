#ifndef RANDOMIZEDSENDER_H
#define RANDOMIZEDSENDER_H

#include <string>
#include <mutex>
#include <unordered_map>
#include "UdpComm.h"  // Make sure this file exists in your project

class RandomizedSender {
public:
    // Constructor takes the device name, a const reference to the devices map, and a pointer to the shared UdpComm instance.
    RandomizedSender(const std::string &deviceName,
                     const std::unordered_map<std::string, std::string> &devices,
                     UdpComm *udp);

    void setOnOff(bool state);
    void scheduleNext();

private:
    std::string generateRandomCommand();
    void sendUdpMessage(const std::string &command);

    std::string deviceName;
    std::string deviceIp;      // Looked up using the devices map
    bool dots_on;              // When true, the sender will actually send commands
    std::mutex mtx;
    UdpComm *udp;              // Shared UdpComm instance for sending messages
};

#endif // RANDOMIZEDSENDER_H
