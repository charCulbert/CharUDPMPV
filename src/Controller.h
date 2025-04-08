#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include <unordered_map>
#include <vector>
#include "json.hpp"
#include "UdpComm.h"
#include <netinet/in.h>

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

    // Cue definitions (each cue is a JSON object).
    std::vector<json> cues;

    // Start the controller (spawns a UDP listener in its own thread).
    void start();

private:
    // UdpComm instance dedicated to controller operations.
    UdpComm *udp;

    // Process an incoming UDP message.
    void processIncomingMessage(const std::string &msg, const sockaddr_in &src, socklen_t srcLen);
};

#endif // CONTROLLER_H
