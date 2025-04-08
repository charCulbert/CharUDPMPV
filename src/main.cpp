#include <mpv/client.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "UdpComm.h"
#include "CommandProcessor.h"

// Global video control variables.
std::string current_video = "";
std::string attract_video = "attract.mp4";
bool use_attract = true;
bool default_loops = false;

// UDP configuration.
int udp_listen_port = 12345;         // Port to receive commands.
int udp_send_port = 12346;           // Port to send log/status messages.
std::string controller_ip = "192.168.1.100"; // Controller IP.

// Reads configuration from "player.conf".
void read_config() {
    std::ifstream infile("player.conf");
    if (!infile) {
        std::cerr << "Config file not found, using default settings.\n";
        return;
    }
    std::string line;
    auto trim = [](std::string s) -> std::string {
        const char* whitespace = " \t\n\r";
        s.erase(0, s.find_first_not_of(whitespace));
        s.erase(s.find_last_not_of(whitespace) + 1);
        return s;
    };
    while(getline(infile, line)) {
        if(line.empty() || line[0]=='#')
            continue;
        size_t pos = line.find('=');
        if(pos == std::string::npos)
            continue;
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos+1));
        if(key == "udp_listen_port")
            udp_listen_port = std::stoi(value);
        else if(key == "udp_send_port")
            udp_send_port = std::stoi(value);
        else if(key == "controller_ip")
            controller_ip = value;
        else if(key == "use_attract")
            use_attract = (value=="true" || value=="True");
        else if(key == "default_loops")
            default_loops = (value=="true" || value=="True");
        else if(key == "attract_video")
            attract_video = value;
    }
}

// Utility: trim trailing newline/carriage return.
void trim_trailing(std::string &s) {
    while(!s.empty() && (s.back()=='\n' || s.back()=='\r'))
        s.pop_back();
}

// MPV utility functions used by CommandProcessor.

void set_option(mpv_handle* ctx, const char* name, const char* value, UdpComm &udp) {
    int error = mpv_set_option_string(ctx, name, value);
    if(error < 0)
        udp.sendLog(std::string("Failed to set option '") + name + "' to '" + value +
                 "': " + mpv_error_string(error));
    else
        udp.sendLog(std::string("Set option '") + name + "' to '" + value + "'");
}

void load_file_command(mpv_handle* ctx, const std::string &filename, bool auto_resume, UdpComm &udp) {
    current_video = filename;
    const char* cmd[] = {"loadfile", filename.c_str(), "replace", NULL};
    int status = mpv_command(ctx, cmd);
    if(status < 0)
        udp.sendLog("Error loading file " + filename + ": " + mpv_error_string(status));
    else {
        udp.sendLog("Loaded file: " + filename);
        if(auto_resume) {
            const char* play_cmd[] = {"set", "pause", "no", NULL};
            mpv_command(ctx, play_cmd);
            udp.sendLog("Resuming playback.");
        }
    }
}

void set_loops(mpv_handle* ctx, bool loop, UdpComm &udp) {
    if(loop) {
        set_option(ctx, "loop-file", "inf", udp);
        udp.sendLog("Looping enabled.");
    } else {
        set_option(ctx, "loop-file", "0", udp);
        udp.sendLog("Looping disabled.");
    }
}

void print_controls(UdpComm &udp) {
    std::string controls =
        "\n=== MPV Player Controls ===\n"
        "q               - Quit\n"
        "Space           - Play/Pause\n"
        "f               - Toggle fullscreen\n"
        "1               - Jump to first frame\n"
        "←/→             - Frame-step (when paused)\n"
        "[Mouse]         - Click and drag to move window\n"
        "[Mouse]         - Double click for fullscreen\n"
        "----------------------------\n"
        "UDP Commands:\n"
        "   LOAD {FILENAME}\n"
        "   LOOPS {FILENAME}\n"
        "   PLAY {FILENAME}\n"
        "   PLAY\n"
        "   STOP\n"
        "   SEEK <time>\n"
        "   VOL <number>\n"
        "   FINAL HOLD\n"
        "   FINAL NOTHING\n"
        "   SETLOOPS ON\n"
        "   SETLOOPS OFF\n"
        "   REMOVE or UNLOAD\n"
        "   ATTRACT {FILENAME}\n"
        "   USEATTRACT ON\n"
        "   USEATTRACT OFF\n"
        "   STATUS\n"
        "==========================================\n";
    udp.sendLog(controls);
}

int main() {
    read_config();
    UdpComm udp(udp_listen_port, udp_send_port, controller_ip);
    udp.sendLog("Using UDP listen port: " + std::to_string(udp_listen_port));
    udp.sendLog("Using UDP send port: " + std::to_string(udp_send_port));
    udp.sendLog("Controller IP: " + controller_ip);
    udp.sendLog("use_attract: " + std::string(use_attract ? "true" : "false"));
    udp.sendLog("default_loops: " + std::string(default_loops ? "true" : "false"));
    udp.sendLog("attract_video: " + attract_video);

    // Create MPV context.
    mpv_handle *ctx = mpv_create();
    if(!ctx) {
        udp.sendLog("Failed to create MPV context");
        return 1;
    }
    set_option(ctx, "input-terminal", "no", udp);
    set_option(ctx, "terminal", "no", udp);
    set_option(ctx, "input-vo-keyboard", "yes", udp);
    set_option(ctx, "input-default-bindings", "yes", udp);
    set_option(ctx, "force-window", "yes", udp);
    set_option(ctx, "border", "no", udp);
    set_option(ctx, "keep-open", "no", udp);
    set_option(ctx, "autofit", "500x500", udp);
    set_option(ctx, "screen", "1", udp);
    set_option(ctx, "window-dragging", "yes", udp);
    set_option(ctx, "demuxer-max-bytes", "1GiB", udp);
    set_option(ctx, "demuxer-max-back-bytes", "1GiB", udp);
    set_option(ctx, "loop-file", "0", udp);
    set_option(ctx, "hr-seek", "yes", udp);
    set_option(ctx, "hr-seek-framedrop", "no", udp);
    set_option(ctx, "resume-playback", "no", udp);
    set_option(ctx, "hwdec", "auto-safe", udp);
    set_option(ctx, "volume", "100", udp);
    set_option(ctx, "volume-max", "100", udp);
    set_option(ctx, "osd-level", "0", udp);

    int status = mpv_initialize(ctx);
    if(status < 0) {
        udp.sendLog("Failed to initialize MPV: " + std::string(mpv_error_string(status)));
        mpv_terminate_destroy(ctx);
        return 1;
    }
    set_loops(ctx, default_loops, udp);
    print_controls(udp);

    // Run UDP listener in a separate thread.
    std::thread listenerThread([&]() {
        udp.runListener([&](const std::string &cmd, const sockaddr_in &src, socklen_t srcLen) {
            CommandProcessor::processCommand(cmd, ctx, udp, src, srcLen);
        });
    });
    listenerThread.detach();

    udp.sendLog("MPV Initialized. Waiting for events or quit signal...");

    // Main MPV event loop.
    while(true) {
         mpv_event *event = mpv_wait_event(ctx, -1.0);
         if(event->event_id == MPV_EVENT_NONE)
            continue;
         if(event->event_id == MPV_EVENT_SHUTDOWN) {
            udp.sendLog("Received MPV shutdown event. Exiting.");
            break;
         }
         if(event->event_id == MPV_EVENT_LOG_MESSAGE) {
             auto msg = reinterpret_cast<mpv_event_log_message*>(event->data);
             std::string logMsg = "[mpv] " + std::string(msg->prefix) + ": " +
                                  std::string(msg->level) + ": " + std::string(msg->text);
             udp.sendLog(logMsg);
             continue;
         }
         if(event->event_id == MPV_EVENT_END_FILE) {
             mpv_event_end_file* eef = (mpv_event_end_file*)event->data;
             if(eef->reason == MPV_END_FILE_REASON_ERROR && eef->error != 0) {
                  udp.sendLog("MPV Error: Playback terminated with error: " + std::string(mpv_error_string(eef->error)));
             }
             else if(eef->reason == MPV_END_FILE_REASON_EOF) {
                  if(use_attract && (current_video != attract_video)) {
                      udp.sendLog("Main video finished. Playing attract video.");
                      set_loops(ctx, true, udp);
                      load_file_command(ctx, attract_video, true, udp);
                  } else {
                      udp.sendLog("Attract video finished or not used.");
                  }
             }
         }
    }
    udp.sendLog("Terminating MPV...");
    mpv_terminate_destroy(ctx);
    udp.sendLog("MPV Terminated.");
    return 0;
}
