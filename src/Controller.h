#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include "json.hpp"
#include "UdpComm.h"
#include "RandomizedSender.h"


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif


using json = nlohmann::json;

class Controller {
public:
    // Constructor & Destructor.
    Controller();
    ~Controller();

    // Public configuration and state.
    int udp_listen_port;    // UDP receiving port.
    int udp_send_port;      // UDP sending port.
    std::string controller_ip;  // Used as the broadcast/destination IP.

    // Map of device names to their IP addresses.
    std::unordered_map<std::string, std::string> devices;

    // cues fired count
    std::unordered_map<std::string, int> cueFiredCount;
    std::mutex dotsMutex;
    bool dots_on = false;  // Shared flag to control command sending.
    RandomizedSender* dotsBS1 = nullptr;
    RandomizedSender* dotsBS2 = nullptr;

    // Cue definitions (each cue is a JSON object).
    std::vector<json> cues;




    // Start the controller (spawns a UDP listener in its own thread).
    void start();

private:
    // UdpComm instance dedicated to controller operations.
    UdpComm *udp;

    // run the startup commands specified in the json
    void processStartupComplete();

    // Process an incoming UDP message.
    void processIncomingMessage(const std::string &msg, const sockaddr_in &src, socklen_t srcLen);
};

#endif // CONTROLLER_H
