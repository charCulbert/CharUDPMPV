#include "RandomizedSender.h"
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>

RandomizedSender::RandomizedSender(const std::string &deviceName, const std::string &deviceIp, UdpComm *udp)
    : deviceName(deviceName), deviceIp(deviceIp), udp(udp), dots_on(false)
{
}

void RandomizedSender::setOnOff(bool state) {
    std::lock_guard<std::mutex> lock(mutex);
    dots_on = state;
    if (dots_on == false) {
        sendUdpMessage("STOPCL");
    }
}

void RandomizedSender::scheduleNext() {
    // Sleep for a randomized amount of time (5 to 15 seconds)
    std::this_thread::sleep_for(std::chrono::milliseconds(5000 + rand() % 10000));

    // Generate a random command (for example, "PLAY DOTS-a.mp4")
    std::string command = generateRandomCommand();

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!dots_on) {
            return;  // If disabled, exit without sending anything.
        }
    }

    sendUdpMessage(command);
}

std::string RandomizedSender::generateRandomCommand() {
    char randomChar = 'a' + rand() % 23;  // Random letter between 'a' and 'w'
    return "PLAY DOTS-" + std::string(1, randomChar) + ".mp4";
}

void RandomizedSender::sendUdpMessage(const std::string &command) {
    std::cout << "Sending to " << deviceName << " (" << deviceIp << "): " << command << std::endl;
    // Use the provided UDP instance's sendPort.
    udp->sendUdpMessage(command, deviceIp, udp->getSendPort());
}
