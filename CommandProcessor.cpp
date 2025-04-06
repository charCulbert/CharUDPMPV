#include "CommandProcessor.h"
#include <sstream>
#include <iostream>
#include <cctype>
#include <sys/socket.h>
#include <unistd.h>

// Declare external MPV utility functions (defined in main.cpp).
extern void set_option(mpv_handle* ctx, const char* name, const char* value, UdpComm &udp);
extern void load_file_command(mpv_handle* ctx, const std::string &filename, bool auto_resume, UdpComm &udp);
extern void set_loops(mpv_handle* ctx, bool loop, UdpComm &udp);
// Global variables defined in main.cpp.
extern std::string current_video;
extern std::string attract_video;
extern bool use_attract;

void CommandProcessor::processCommand(const std::string &cmd, mpv_handle* ctx, UdpComm &udp,
                                        const sockaddr_in &src, socklen_t srcLen) {
    std::istringstream iss(cmd);
    std::string token;
    iss >> token;

    if (token == "STATUS") {
        std::string reply = "READY";
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock >= 0) {
            sendto(sock, reply.c_str(), reply.size(), 0, (struct sockaddr*)&src, srcLen);
            close(sock);
        }
        udp.sendLog("Sent status reply: " + reply);
    }
    else if (token == "ATTRACT") {
        std::string filename;
        getline(iss, filename);
        while (!filename.empty() && std::isspace(filename.front()))
            filename.erase(filename.begin());
        attract_video = filename;
        udp.sendLog("Attract video set to: " + attract_video);
    }
    else if (token == "USEATTRACT") {
        std::string state;
        iss >> state;
        if (state == "ON") {
            use_attract = true;
            udp.sendLog("Attract feature enabled.");
        } else if (state == "OFF") {
            use_attract = false;
            udp.sendLog("Attract feature disabled.");
        }
    }
    else if (token == "LOOPS") {
        std::string filename;
        getline(iss, filename);
        while (!filename.empty() && std::isspace(filename.front()))
            filename.erase(filename.begin());
        set_loops(ctx, true, udp);
        load_file_command(ctx, filename, true, udp);
    }
    else if (token == "PLAY") {
        std::string filename;
        if (iss >> filename) {
            set_loops(ctx, false, udp);
            load_file_command(ctx, filename, true, udp);
        } else {
            const char* cmd_arr[] = {"set", "pause", "no", NULL};
            mpv_command(ctx, cmd_arr);
            udp.sendLog("Resuming playback.");
        }
    }
    else if (token == "LOAD") {
        std::string filename;
        getline(iss, filename);
        while (!filename.empty() && std::isspace(filename.front()))
            filename.erase(filename.begin());
        load_file_command(ctx, filename, false, udp);
    }
    else if (token == "STOP") {
        const char* cmd_arr[] = {"set", "pause", "yes", NULL};
        mpv_command(ctx, cmd_arr);
        udp.sendLog("Pausing playback.");
    }
    else if (token == "SEEK") {
        double time_val;
        if (!(iss >> time_val))
            udp.sendLog("Invalid SEEK command. Use: SEEK <time_in_seconds>");
        else {
            std::string timeStr = std::to_string(time_val);
            const char* cmd_arr[] = {"seek", timeStr.c_str(), "absolute", NULL};
            int status = mpv_command(ctx, cmd_arr);
            udp.sendLog("Seeking to " + std::to_string(time_val) + " seconds.");
            if (status < 0)
                udp.sendLog("Error executing seek: " + std::string(mpv_error_string(status)));
        }
    }
    else if (token == "VOL") {
        int vol_val;
        if (!(iss >> vol_val))
            udp.sendLog("Invalid VOL command. Use: VOL <number_0_to_100>");
        else {
            if (vol_val < 0) vol_val = 0;
            if (vol_val > 100) vol_val = 100;
            std::string volStr = std::to_string(vol_val);
            const char* cmd_arr[] = {"set", "volume", volStr.c_str(), NULL};
            int status = mpv_command(ctx, cmd_arr);
            udp.sendLog("Setting volume to " + std::to_string(vol_val) + ".");
            if (status < 0)
                udp.sendLog("Error executing volume command: " + std::string(mpv_error_string(status)));
        }
    }
    else if (token == "FINAL") {
        std::string sub;
        iss >> sub;
        if (sub == "HOLD") {
            set_option(ctx, "keep-open", "yes", udp);
        } else if (sub == "NOTHING") {
            set_option(ctx, "keep-open", "no", udp);
        }
    }
    else if (token == "SETLOOPS") {
        std::string state;
        iss >> state;
        if (state == "ON")
            set_loops(ctx, true, udp);
        else if (state == "OFF")
            set_loops(ctx, false, udp);
    }
    else if (token == "REMOVE" || token == "UNLOAD") {
        const char* unload_cmd[] = {"playlist-remove", "current", NULL};
        int status = mpv_command(ctx, unload_cmd);
        if (status < 0)
            udp.sendLog("Error unloading current video: " + std::string(mpv_error_string(status)));
        else
            udp.sendLog("Unloaded current video (removed playlist entry).");
    }
    else {
        udp.sendLog("Unknown UDP command: \"" + cmd + "\"");
    }
}
