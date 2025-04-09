#include "Controller.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>

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
    listenerThread.join();
}

void Controller::processIncomingMessage(const std::string &msg, const sockaddr_in &src, socklen_t srcLen) {
    // Get the sender's IP address as a string.
    char senderIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src.sin_addr, senderIP, sizeof(senderIP));
    std::string senderName = "Unknown";
    std::cout << "senderIP: " << senderIP << std::endl;
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
    for (auto &cue : cues) {
        // We're only handling cues with trigger type "udp_message".
        if (cue.contains("trigger") && cue["trigger"].contains("type") &&
            cue["trigger"]["type"] == "udp_message")
        {
            std::string triggerMessage = cue["trigger"].value("message", "");
            std::string triggerFrom = cue["trigger"].value("from_device", "");
            int triggerDelay = cue["trigger"].value("delay_ms", 0);
            bool resetOnFire = cue["trigger"].value("reset_on_fire", false);
            // (Optional: use "count" if you want to count occurrences before firing.)

            // For this example, match if the incoming message equals the trigger's message
            // and the sender's device name matches.
            if (msg == triggerMessage && senderName == triggerFrom) {
                // udp->sendLog("Controller: Cue '" + cue.value("name", "unnamed") + "' triggered.");

                // Optionally apply trigger delay.
                if (triggerDelay > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(triggerDelay));
                }

                // Process each action in the cue.
                if (cue.contains("actions") && cue["actions"].is_array()) {
                    for (auto &action : cue["actions"]) {
                        if (action.contains("type") && action["type"] == "send_udp") {
                            std::string actionMessage = action.value("message", "");
                            int actionDelay = action.value("delay_ms", 0);

                            // If an action delay is specified, wait before sending.
                            if (actionDelay > 0) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(actionDelay));
                            }

                            // Send the UDP message to each destination device specified.
                            if (action.contains("destination") && action["destination"].is_array()) {
                                for (auto &dest : action["destination"]) {
                                    std::string destName = dest.get<std::string>();
                                    // Look up the device's IP from our devices mapping.
                                    if (devices.find(destName) != devices.end()) {
                                        std::string destIp = devices[destName];
                                        // udp->sendLog("Controller: Sending UDP message '" + actionMessage +
                                        //              "' to " + destName + " (" + destIp + ")");
                                        udp->sendUdpMessage(actionMessage, destIp, udp_send_port);
                                    } else {
                                        // udp->sendLog("Controller: Unknown destination device: " + destName);
                                    }
                                }
                            }
                        }
                    }
                }
                // Optionally, if "reset_on_fire" is false, you could prevent firing the cue again
                // until certain conditions are met. (Not implemented in this minimal example.)
            }
        }
    }
}
