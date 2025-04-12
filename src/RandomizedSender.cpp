#include "RandomizedSender.h"
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <random>

RandomizedSender::RandomizedSender(const std::string &deviceName, const std::string &deviceIp, UdpComm *udp)
    : deviceName(deviceName), deviceIp(deviceIp), udp(udp), dots_on(false), gen(std::random_device{}())
{
    currentSequenceClipsRemaining = 0;
}

void RandomizedSender::setOnOff(bool state) {
    std::lock_guard<std::mutex> lock(mutex);
    dots_on = state;
    if (dots_on == false) {
        sendUdpMessage("STOPCL");
    }
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
        int waitMillis;
        {
            // Guard random engine access with genMutex.
            std::lock_guard<std::mutex> lock(genMutex);
            std::uniform_int_distribution<int> waitDist(30000, 170001);
            waitMillis = waitDist(gen);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(waitMillis));

        // Generate a new sequence length between 1 and 10.
        {
            std::lock_guard<std::mutex> lock(genMutex);
            std::uniform_int_distribution<int> seqDist(1, 10);
            currentSequenceClipsRemaining = seqDist(gen);
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!dots_on) {
                return; // Don't proceed if disabled.
            }
        }
        // Play the first clip of the new sequence.
        std::string command = generateRandomCommand();
        sendUdpMessage(command);
        currentSequenceClipsRemaining--;
    }
}


std::string RandomizedSender::generateRandomCommand() {
    char randomChar;
    {
        // Guard generator when picking a random command.
        std::lock_guard<std::mutex> lock(genMutex);
        std::uniform_int_distribution<int> dis(0, 22);
        randomChar = 'a' + dis(gen);
    }
    return "PLAY DOTS-" + std::string(1, randomChar) + ".mp4";
}


void RandomizedSender::sendUdpMessage(const std::string &command) {
    std::cout << "Sending to " << deviceName << " (" << deviceIp << "): " << command << std::endl;
    // Use the provided UDP instance's sendPort.
    udp->sendUdpMessage(command, deviceIp, udp->getSendPort());
}
