#include "Controller.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
typedef int socklen_t;
typedef long ssize_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#endif


Controller::Controller()
    : udp_listen_port(12346),    // You can adjust these defaults as needed.
      udp_send_port(12345),
      controller_ip("255.255.255.255"),
      udp(nullptr)

{
}

Controller::~Controller() {
    // If necessary, add a mechanism to stop the UDP listener.
    if (udp) {
        delete udp;
    }
    if (dotsBS1) {
        delete dotsBS1;
    }
    if (dotsBS2) {
        delete dotsBS2;
    }
}
void Controller::processStartupComplete() {
    for (auto &cue : cues) {
        if (cue.contains("trigger") && cue["trigger"].contains("type") &&
            cue["trigger"]["type"] == "startup_complete")
        {
            std::string cueName = cue["name"].get<std::string>();
            std::cout << "Startup cue triggered: " << cueName << std::endl;

            // Immediately process each action in the cue.
            if (cue.contains("actions") && cue["actions"].is_array()) {
                for (auto &action : cue["actions"]) {
                    if (action.contains("type") && action["type"] == "send_udp") {
                        std::string actionMessage = action.value("message", "");
                        // Send UDP message to each destination immediately.
                        if (action.contains("destination") && action["destination"].is_array()) {
                            for (auto &dest : action["destination"]) {
                                std::string destName = dest.get<std::string>();
                                if (devices.find(destName) != devices.end()) {
                                    std::string destIp = devices.at(destName);
                                    udp->sendUdpMessage(actionMessage, destIp, udp_send_port);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Now load the RandomizedSender instances for dotsBS1 and dotsBS2.
    auto it1 = devices.find("BS1");
    if (it1 != devices.end()) {
        dotsBS1 = new RandomizedSender("dotsBS1", it1->second, udp);
        std::cout << "Initialized RandomizedSender dotsBS1" << std::endl;
    } else {
        std::cout << "Device dotsBS1 not found in devices list." << std::endl;
    }

    auto it2 = devices.find("BS2");
    if (it2 != devices.end()) {
        dotsBS2 = new RandomizedSender("dotsBS2", it2->second, udp);
        std::cout << "Initialized RandomizedSender dotsBS2" << std::endl;
    } else {
        std::cout << "Device dotsBS2 not found in devices list." << std::endl;
    }
}


void Controller::start() {
    // Create the UdpComm instance using the controller’s configuration.
    udp = new UdpComm(udp_listen_port, udp_send_port, controller_ip);
    // udp->sendLog("Controller: UDP Listener started on port " + std::to_string(udp_listen_port));

    // Start the UDP listener in a separate thread.
    std::thread listenerThread([this]() {
        udp->runListener([this](const std::string &msg, const sockaddr_in &src, socklen_t srcLen) {
            processIncomingMessage(msg, src, srcLen);
        });
    });
    std::cout << "waiting 3s for initialisation before running startup commands" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    processStartupComplete(); // or whatever your device name is

    dotsBS1->setOnOff(true);
    dotsBS2->setOnOff(true);
    std::thread([this]() { dotsBS1->scheduleNext(); }).detach();
    std::thread([this]() { dotsBS2->scheduleNext(); }).detach();

    listenerThread.join();
}

void Controller::processIncomingMessage(const std::string &msg, const sockaddr_in &src, socklen_t srcLen) {
    // Get the sender's IP address as a string.
    char senderIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src.sin_addr, senderIP, sizeof(senderIP));
    std::string senderName = "Unknown";
    // std::cout << "senderIP: " << senderIP << std::endl;
    // Print all devices in the map to check if they're properly populated
    for (const auto &d : devices) {
        // Use a more robust comparison
        if (std::string(senderIP) == d.second) {
            senderName = d.first;
            break;
        }
    }


    std::cout << senderName << "says: " << msg << std::endl;


    // Iterate over cues to check if any should be triggered by this message.

    // Check each cue to see if it should fire.
    for (auto &cue : cues) {
        // Must have trigger type "udp_message" to handle here.
        if (cue.contains("trigger") && cue["trigger"].contains("type") &&
            cue["trigger"]["type"] == "udp_message")
        {
            std::string triggerMessage = cue["trigger"].value("message", "");
            std::string triggerFrom    = cue["trigger"].value("from_device", "");
            int triggerDelay           = cue["trigger"].value("delay_ms", 0);

            // Optional: how often to do "alternate_actions"
            // If not set, default to 1 (meaning "alternate_actions" never used unless 1 is also doing that logic).
            int countRequirement = cue["trigger"].value("count", 1);

            // Check if message & sender match
            if (msg == triggerMessage && senderName == triggerFrom) {
                // Cue triggered.
                std::string cueName = cue["name"].get<std::string>();
                std::cout << "cue triggered: " << cueName << std::endl;

                // Increment the times this cue has fired.
                cueFiredCount[cueName]++;

                // Fire the cue in a separate thread for delay & concurrency.
                std::thread([=]() {
                    // Delay if specified
                    if (triggerDelay > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(triggerDelay));
                    }

                    // Determine if we use alternate_actions
                    int firedSoFar   = cueFiredCount.at(cueName);
                    bool useAlternate = false;


                    if (cue.contains("alternate_actions") && cue["alternate_actions"].is_array()) {
                        std::cout << "firedsofar is: " << firedSoFar << std::endl;
                        if (firedSoFar == countRequirement) {
                      useAlternate = true;
                      cueFiredCount[cueName] = 0;
                  }
                        std::cout << "using alternate? " << useAlternate << std::endl;

                    }

                    // If "alternate_actions" array is present and it's time to use it
                    if (useAlternate && cue.contains("alternate_actions") && cue["alternate_actions"].is_array()) {
                        for (auto &action : cue["alternate_actions"]) {
                            if (action.contains("type") && action["type"] == "send_udp") {
                                std::string actionMessage = action.value("message", "");
                                int actionDelay           = action.value("delay_ms", 0);
                                if (actionDelay > 0) {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(actionDelay));
                                }
                                // Send to each destination
                                if (action.contains("destination") && action["destination"].is_array()) {
                                    for (auto &dest : action["destination"]) {
                                        std::string destName = dest.get<std::string>();
                                        if (devices.find(destName) != devices.end()) {
                                            std::string destIp = devices.at(destName);
                                            udp->sendUdpMessage(actionMessage, destIp, udp_send_port);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // Otherwise, default to normal "actions" if it exists
                    else if (cue.contains("actions") && cue["actions"].is_array()) {
                        for (auto &action : cue["actions"]) {
                            if (action.contains("type") && action["type"] == "send_udp") {
                                std::string actionMessage = action.value("message", "");
                                int actionDelay           = action.value("delay_ms", 0);
                                if (actionDelay > 0) {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(actionDelay));
                                }
                                // Send to each destination
                                if (action.contains("destination") && action["destination"].is_array()) {
                                    for (auto &dest : action["destination"]) {
                                        std::string destName = dest.get<std::string>();
                                        if (devices.find(destName) != devices.end()) {
                                            std::string destIp = devices.at(destName);
                                            udp->sendUdpMessage(actionMessage, destIp, udp_send_port);
                                        }
                                    }
                                }
                            }
                        }
                    }

                }).detach();
            }
        }
    }
    // NEW: Check if the message is "ENDP" and call scheduleNext on the corresponding RandomizedSender
    if (msg == "ENDP") {
        if (senderName == "BS1") {
            if (dotsBS1) {
                dotsBS1->sendUdpMessage("STOPCL");
                std::thread([this]() { dotsBS1->scheduleNext(); }).detach();
            }
        }
        else if (senderName == "BS2") {
            if (dotsBS2) {
                dotsBS2->sendUdpMessage("STOPCL");
                std::thread([this]() { dotsBS2->scheduleNext(); }).detach();
            }
        }
    }
        if (msg == "EOF") {
            if (senderName == "AnaPC") {
                if (dotsBS1)
                    dotsBS1->setOnOff(false);
                if (dotsBS2)
                    dotsBS2->setOnOff(false);
            }
        }
        if (msg == "EOF") {
            if (senderName == "VIDEOPC2") {
                if (dotsBS1)
                    dotsBS1->setOnOff(true);
                std::thread([this]() { dotsBS1->scheduleNext(); }).detach();
                if (dotsBS2)
                    dotsBS2->setOnOff(true);
                std::thread([this]() { dotsBS2->scheduleNext(); }).detach();
            }
        }
    }

