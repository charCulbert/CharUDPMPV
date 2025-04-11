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
    int currentSequenceClipsRemaining;

}

void RandomizedSender::scheduleNext() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!dots_on) {
            return; // Don't proceed if disabled.
        }
    }

    // Check if there are remaining clips in the current sequence.
    if (currentSequenceClipsRemaining > 0) {
        std::cout << "sequence clips remaining" << currentSequenceClipsRemaining << std::endl;
        // There are clips remaining in the sequence: play next clip immediately.
        std::string command = generateRandomCommand();
        sendUdpMessage(command);
        currentSequenceClipsRemaining--;
    } else {
        // No more clips in the current sequence.
        // Wait a random period between 10 and 100 seconds before starting a new sequence.
        int waitMillis = 10000 + rand() % 90000; // 10,000 to 100,000 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(waitMillis));

        // Generate a new sequence length (1 to 10 clips).
        currentSequenceClipsRemaining = 1 + rand() % 10;


        // Play the first clip of the new sequence.
        std::string command = generateRandomCommand();
        sendUdpMessage(command);
        currentSequenceClipsRemaining--;
    }
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
