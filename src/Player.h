#ifndef PLAYER_H
#define PLAYER_H

#include <mpv/client.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "json.hpp"
#include "UdpComm.h"

using json = nlohmann::json;

class Player {
public:
    // Constructor & Destructor.
    Player();
    ~Player();

    // Public configuration and state.
    std::string current_video;
    std::string attract_video;
    std::string player_name;
    bool use_attract;
    bool altEOF_mode; // true if current video was launched with ALTPLAY

    int udp_listen_port;  // UDP receiving port.
    int udp_send_port;    // UDP sending port.
    std::string controller_ip;

    std::unordered_map<std::string, std::string> devices;
    std::vector<json> cues;

    // Start the player (initializes MPV, configures, and runs event and UDP loops).
    void start();

    // Command-processing method called when a UDP command is received.
    void processCommand(const std::string &cmd, UdpComm &udp, const sockaddr_in &src, socklen_t srcLen);

    // Utility methods.
    void setOption(mpv_handle* ctx, const char* name, const char* value, UdpComm &udp);
    void loadFileCommand(mpv_handle* ctx, const std::string &filename, bool auto_resume, UdpComm &udp);
    void setLoops(mpv_handle* ctx, bool loop, UdpComm &udp);
    void printControls(UdpComm &udp);

private:
    mpv_handle *ctx;  // MPV context.

};

#endif // PLAYER_H
