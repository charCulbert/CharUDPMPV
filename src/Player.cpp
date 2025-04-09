#include "Player.h"
#include <iostream>
#include <fstream>
#include <cstring>


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <thread>

Player::Player()
    : current_video(""),
      attract_video("attract.mp4"),
      use_attract(true),
      udp_listen_port(12345),
      udp_send_port(12346),
      controller_ip("192.168.1.100"),
      player_name("default"),
      ctx(nullptr)
{
    // Optionally, initialize other members here.
}

Player::~Player() {
    if (ctx) {
        mpv_terminate_destroy(ctx);
    }
}

void Player::setOption(mpv_handle* ctx, const char* name, const char* value, UdpComm &udp) {
    int error = mpv_set_option_string(ctx, name, value);
    if (error < 0)
        udp.sendLog(std::string("Failed to set option '") + name + "' to '" + value +
                    "': " + mpv_error_string(error));
    // else
        // udp.sendLog(std::string("Set option '") + name + "' to '" + value + "'");
}

void Player::loadFileCommand(mpv_handle* ctx, const std::string &filename, bool auto_resume, UdpComm &udp) {
    current_video = filename;
    const char* cmd[] = {"loadfile", filename.c_str(), "replace", nullptr};
    int status = mpv_command(ctx, cmd);
    if (status < 0)
        udp.sendLog("Error loading file " + filename + ": " + mpv_error_string(status));
    else {
        udp.sendLog("Loaded file: " + filename);
        if (auto_resume) {
            const char* play_cmd[] = {"set", "pause", "no", nullptr};
            mpv_command(ctx, play_cmd);
            udp.sendLog("Resuming playback.");
        }
    }
}


//
// void Player::printControls(UdpComm &udp) {
//     std::string controls =
//         "\n=== MPV Player Controls ===\n"
//         "q               - Quit\n"
//         "Space           - Play/Pause\n"
//         "f               - Toggle fullscreen\n"
//         "1               - Jump to first frame\n"
//         "←/→             - Frame-step (when paused)\n"
//         "[Mouse]         - Click and drag to move window\n"
//         "[Mouse]         - Double click for fullscreen\n"
//         "----------------------------\n"
//         "UDP Commands:\n"
//         "   ATTRACT {FILENAME}\n"
//         "   PLAY\n"
//         "   SETLOOP [ON/OFF]\n"
//         "==========================================\n";
//     udp.sendLog(controls);
// }

void Player::processCommand(const std::string &cmd, UdpComm &udp, const sockaddr_in &src, socklen_t srcLen) {

    // LOAD {FILENAME} command: Load file with no explicit looping.
    if (cmd.substr(0, 5) == "LOAD ") {
        std::string filename = cmd.substr(5);
        udp.sendLog("LOAD command. Filename: " + filename);
        loadFileCommand(ctx, filename, true, udp);
    }
    // LOOPS {FILENAME} command: Load file with looping enabled.
    else if (cmd.substr(0, 6) == "LOOPS") {
        // Expecting "LOOPS {FILENAME}"
        std::string filename = cmd.substr(7);
        udp.sendLog("LOOPS command. Filename: " + filename);
        setOption(ctx, "loop-file", "inf", udp);
        loadFileCommand(ctx, filename, true, udp);
    }
    // PLAY {FILENAME} command: Load file and play it.
    else if (cmd.substr(0, 5) == "PLAY " && cmd.size() > 5) {
        std::string filename = cmd.substr(5);
        udp.sendLog("PLAY command with filename: " + filename);
        loadFileCommand(ctx, filename, true, udp);
    }
    // PLAY (without argument): Resume current video.
    else if (cmd == "PLAY") {
        udp.sendLog("PLAY command received (resuming playback).");
        const char* play_cmd[] = {"set", "pause", "no", nullptr};
        mpv_command(ctx, play_cmd);
    }
    // STOP command: Pause playback.
    else if (cmd == "STOP") {
        udp.sendLog("STOP command received (pausing playback).");
        const char* stop_cmd[] = {"set", "pause", "yes", nullptr};
        mpv_command(ctx, stop_cmd);
    }
    // SEEK <time> command: Seek to the specified time.
    else if (cmd.substr(0, 5) == "SEEK ") {
        std::string timeStr = cmd.substr(5);
        udp.sendLog("SEEK command received. Time: " + timeStr);
        // Build command: seek <time> absolute
        const char* seek_cmd[] = {"seek", timeStr.c_str(), "absolute", nullptr};
        mpv_command(ctx, seek_cmd);
    }
    // VOL <number> command: Set volume.
    else if (cmd.substr(0, 4) == "VOL ") {
        std::string volStr = cmd.substr(4);
        udp.sendLog("VOL command received. Volume: " + volStr);
        setOption(ctx, "volume", volStr.c_str(), udp);
    }
    // FINAL HOLD command.
    else if (cmd == "FINAL HOLD") {
        udp.sendLog("FINAL HOLD command received.");
        setOption(ctx, "keep-open", "yes", udp);
    }
    // FINAL NOTHING command.
    else if (cmd == "FINAL NOTHING") {
        udp.sendLog("FINAL NOTHING command received.");
        setOption(ctx, "keep-open", "no", udp);
    }
    // SETLOOPS ON / SETLOOP ON command.
    else if (cmd == "SETLOOPS ON" || cmd == "SETLOOP ON") {
        udp.sendLog("SETLOOPS ON command received.");
        setOption(ctx, "loop-file", "inf", udp);
    }
    // SETLOOPS OFF / SETLOOP OFF command.
    else if (cmd == "SETLOOPS OFF" || cmd == "SETLOOP OFF") {
        udp.sendLog("SETLOOPS OFF command received.");
        setOption(ctx, "loop-file", "0", udp);
    }
    // CLEAR or UNLOAD command: Unload the current video.
    else if (cmd == "CLEAR" || cmd == "UNLOAD") {
        udp.sendLog("CLEAR/UNLOAD command received. Unloading current video.");
        // One approach: send a "stop" command.
        const char* unload_cmd[] = {"stop", nullptr};
        mpv_command(ctx, unload_cmd);
        current_video = "";
    }
    // ATTRACT {FILENAME} command.
    else if (cmd.substr(0, 7) == "ATTRACT") {
        // Expecting "ATTRACT {FILENAME}"
        std::string filename = cmd.substr(8); // Assumes a space after ATTRACT.
        udp.sendLog("ATTRACT command. Filename: " + filename);
        setOption(ctx, "loop-file", "inf", udp);
        loadFileCommand(ctx, filename, true, udp);
    }
    // USEATTRACT ON / USEATTRACT OFF: Toggle attract mode.
    else if (cmd == "USEATTRACT ON") {
        use_attract = true;
        udp.sendLog("USEATTRACT ON command received. Attract mode enabled.");
    }
    else if (cmd == "USEATTRACT OFF") {
        use_attract = false;
        udp.sendLog("USEATTRACT OFF command received. Attract mode disabled.");
    }
    // STATUS command: Report current status.
    else if (cmd == "STATUS") {
        udp.sendLog("STATUS command received.");
        udp.sendLog("Current video: " + current_video);
        udp.sendLog("Attract video: " + attract_video);
        udp.sendLog("use_attract: " + std::string(use_attract ? "true" : "false"));
        // Optionally add additional status information.
        udp.sendLog("READY");
    }
    else {
        udp.sendLog("Unrecognized command: " + cmd);
    }
}


void Player::start() {
    // Create a local UdpComm instance using our configuration.
    UdpComm udp(udp_listen_port, udp_send_port, controller_ip);
    udp.sendLog("Hello from player: " + player_name + "\n");

    // Create MPV context.
    ctx = mpv_create();
    if (!ctx) {
        udp.sendLog("Failed to create MPV context");
        return;
    }
    // Set MPV options.
    setOption(ctx, "input-terminal", "no", udp);
    setOption(ctx, "terminal", "no", udp);
    setOption(ctx, "input-vo-keyboard", "yes", udp);
    setOption(ctx, "input-default-bindings", "yes", udp);
    setOption(ctx, "force-window", "yes", udp);
    setOption(ctx, "border", "no", udp);
    setOption(ctx, "keep-open", "no", udp);
    setOption(ctx, "autofit", "500x500", udp);
    setOption(ctx, "screen", "1", udp);
    setOption(ctx, "window-dragging", "yes", udp);
    setOption(ctx, "demuxer-max-bytes", "1GiB", udp);
    setOption(ctx, "demuxer-max-back-bytes", "1GiB", udp);
    setOption(ctx, "loop-file", "0", udp);
    setOption(ctx, "hr-seek", "yes", udp);
    setOption(ctx, "hr-seek-framedrop", "no", udp);
    setOption(ctx, "resume-playback", "no", udp);
    setOption(ctx, "hwdec", "auto-safe", udp);
    setOption(ctx, "volume", "100", udp);
    setOption(ctx, "osd-level", "0", udp);

    int status = mpv_initialize(ctx);
    if (status < 0) {
        udp.sendLog("Failed to initialize MPV: " + std::string(mpv_error_string(status)));
        mpv_terminate_destroy(ctx);
        return;
    }

    // Start the UDP listener in a separate thread.
    std::thread listenerThread([this, &udp]() {
        udp.runListener([this, &udp](const std::string &cmd, const sockaddr_in &src, socklen_t srcLen) {
            this->processCommand(cmd, udp, src, srcLen);
        });
    });
    listenerThread.detach();

    udp.sendLog("MPV Initialized. Waiting for events or quit signal...");

    // Main MPV event loop.
    while (true) {
        mpv_event *event = mpv_wait_event(ctx, -1.0);
        if (event->event_id == MPV_EVENT_NONE)
            continue;
        if (event->event_id == MPV_EVENT_SHUTDOWN) {
            udp.sendLog("Received MPV shutdown event. Exiting.");
            break;
        }
        if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
            auto msg = reinterpret_cast<mpv_event_log_message*>(event->data);
            std::string logMsg = "[mpv] " + std::string(msg->prefix) + ": " +
                                 std::string(msg->level) + ": " + std::string(msg->text);
            udp.sendLog(logMsg);
            continue;
        }
        if (event->event_id == MPV_EVENT_END_FILE) {
            auto eef = reinterpret_cast<mpv_event_end_file*>(event->data);
            if (eef->reason == MPV_END_FILE_REASON_ERROR && eef->error != 0) {
                udp.sendLog("MPV Error: Playback terminated with error: " + std::string(mpv_error_string(eef->error)));
            } else if (eef->reason == MPV_END_FILE_REASON_EOF) {
                if (use_attract && (current_video != attract_video)) {
                    udp.sendLog("EOF");
                    udp.sendLog("Playing attract video.");
                    setOption(ctx, "loop-file", "inf", udp);
                    loadFileCommand(ctx, attract_video, true, udp);
                } else {
                    udp.sendLog("EOF");
                }
            }
        }
    }
    udp.sendLog("Terminating MPV...");
    mpv_terminate_destroy(ctx);
    udp.sendLog("MPV Terminated.");
}
