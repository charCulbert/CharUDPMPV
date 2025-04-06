#include <mpv/client.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

// Global variables for attract feature.
std::string current_video = "";
std::string attract_video = "attract.mp4";
bool use_attract = true;  // Controls whether the attract feature is active.

void set_option(mpv_handle* ctx, const char* name, const char* value) {
    int error = mpv_set_option_string(ctx, name, value);
    if (error < 0) {
        std::cerr << "Failed to set option '" << name << "' to '" << value
                  << "': " << mpv_error_string(error) << std::endl;
    } else {
         std::cout << "Set option '" << name << "' to '" << value << "'\n";
    }
}

// Helper: load a file and update current_video. If auto_resume is true, playback begins immediately.
void load_file_command(mpv_handle* ctx, const std::string &filename, bool auto_resume) {
    current_video = filename;  // Track the current file.
    const char* cmd[] = {"loadfile", filename.c_str(), "replace", NULL};
    int status = mpv_command(ctx, cmd);
    if (status < 0) {
        std::cerr << "Error loading file " << filename << ": "
                  << mpv_error_string(status) << std::endl;
    } else {
        std::cout << "Loaded file: " << filename << std::endl;
        if (auto_resume) {
            const char* play_cmd[] = {"set", "pause", "no", NULL};
            mpv_command(ctx, play_cmd);
            std::cout << "Resuming playback.\n";
        }
    }
}

// Helper: set looping flag.
void set_loops(mpv_handle* ctx, bool loop) {
    if (loop) {
        set_option(ctx, "loop-file", "inf");
        std::cout << "Looping enabled.\n";
    } else {
        set_option(ctx, "loop-file", "0");
        std::cout << "Looping disabled.\n";
    }
}

void print_controls() {
    std::cout << "\n=== MPV Player Controls ===\n"
              << "q               - Quit\n"
              << "Space           - Play/Pause\n"
              << "f               - Toggle fullscreen\n"
              << "1               - Jump to first frame\n"
              << "←/→             - Frame-step (when paused)\n"
              << "[Mouse]         - Click and drag to move window\n"
              << "[Mouse]         - Double click for fullscreen\n"
              << "----------------------------\n"
              << "UDP Commands (Port 12345):\n"
              << "   LOAD {FILENAME}    - Load file (preserving current play state)\n"
              << "   LOOPS {FILENAME}   - Set looping on, load file, and play\n"
              << "   PLAY {FILENAME}    - Set looping off, load file, and play\n"
              << "   PLAY               - Resume playback (if paused)\n"
              << "   STOP               - Pause playback\n"
              << "   SEEK <time>        - Seek to specified time (seconds)\n"
              << "   VOL <number>       - Set volume (0-100)\n"
              << "   FINAL HOLD         - Hold last frame on end of file (do nothing)\n"
              << "   FINAL NOTHING      - Print \"EOF\" on end of file\n"
              << "   SETLOOPS ON        - Enable looping (without loading a file)\n"
              << "   SETLOOPS OFF       - Disable looping (without loading a file)\n"
              << "   REMOVE or UNLOAD   - Unload (remove) current video\n"
              << "   ATTRACT {FILENAME} - Set the attract video\n"
              << "   USEATTRACT ON      - Enable attract feature\n"
              << "   USEATTRACT OFF     - Disable attract feature\n"
              << "==========================================\n\n";
}

void udp_listener(mpv_handle* ctx, int udp_port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating UDP socket: " << strerror(errno) << std::endl;
        return;
    }
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "Warning: setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(udp_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding UDP socket to port " << udp_port
                  << ": " << strerror(errno) << std::endl;
        close(sockfd);
        return;
    }
    std::cout << "UDP Listener started on port " << udp_port << "." << std::endl;
    char buffer[1024];
    while (true) {
        struct sockaddr_in src_addr;
        socklen_t src_len = sizeof(src_addr);
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr*)&src_addr, &src_len);
        if (recv_len < 0) {
             std::cerr << "Error receiving UDP data: " << strerror(errno) << std::endl;
             std::this_thread::sleep_for(std::chrono::milliseconds(100));
             continue;
        }
        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            std::string command(buffer);
            while (!command.empty() && (command.back() == '\n' || command.back() == '\r'))
                command.pop_back();
            std::cout << "UDP Received: \"" << command << "\"" << std::endl;

            // ATTRACT {FILENAME}: set the attract video.
            if (command.rfind("ATTRACT ", 0) == 0) {
                attract_video = command.substr(8);
                std::cout << "Attract video set to: " << attract_video << std::endl;
            }
            // USEATTRACT ON/OFF: enable or disable the attract feature.
            else if (command == "USEATTRACT ON") {
                use_attract = true;
                std::cout << "Attract feature enabled.\n";
            }
            else if (command == "USEATTRACT OFF") {
                use_attract = false;
                std::cout << "Attract feature disabled.\n";
            }
            // LOOPS {FILENAME}: set looping on, load file, and play.
            else if (command.rfind("LOOPS ", 0) == 0) {
                std::string filename = command.substr(6);
                set_loops(ctx, true);
                load_file_command(ctx, filename, true);
            }
            // PLAY {FILENAME}: set looping off, load file, and play.
            else if (command.rfind("PLAY ", 0) == 0) {
                std::string filename = command.substr(5);
                set_loops(ctx, false);
                load_file_command(ctx, filename, true);
            }
            // LOAD {FILENAME}: load file (preserving current play state).
            else if (command.rfind("LOAD ", 0) == 0) {
                std::string filename = command.substr(5);
                load_file_command(ctx, filename, false);
            }
            // PLAY: resume playback if paused.
            else if (command == "PLAY") {
                const char* cmd[] = {"set", "pause", "no", NULL};
                mpv_command(ctx, cmd);
                std::cout << "Resuming playback.\n";
            }
            // STOP: pause playback.
            else if (command == "STOP") {
                const char* cmd[] = {"set", "pause", "yes", NULL};
                mpv_command(ctx, cmd);
                std::cout << "Pausing playback.\n";
            }
            // SEEK command.
            else if (command.rfind("SEEK ", 0) == 0) {
                std::istringstream iss(command);
                std::string token;
                double time_val;
                if (!(iss >> token >> time_val)) {
                    std::cerr << "Invalid SEEK command. Use: SEEK <time_in_seconds>\n";
                } else {
                    std::string timeStr = std::to_string(time_val);
                    const char* cmd[] = {"seek", timeStr.c_str(), "absolute", NULL};
                    int status = mpv_command(ctx, cmd);
                    std::cout << "Seeking to " << time_val << " seconds.\n";
                    if (status < 0)
                        std::cerr << "Error executing seek: " << mpv_error_string(status) << std::endl;
                }
            }
            // VOL command.
            else if (command.rfind("VOL ", 0) == 0) {
                std::istringstream iss(command);
                std::string token;
                int vol_val;
                if (!(iss >> token >> vol_val)) {
                    std::cerr << "Invalid VOL command. Use: VOL <number_0_to_100>\n";
                } else {
                    if (vol_val < 0) vol_val = 0;
                    if (vol_val > 100) vol_val = 100;
                    std::string volStr = std::to_string(vol_val);
                    const char* cmd[] = {"set", "volume", volStr.c_str(), NULL};
                    int status = mpv_command(ctx, cmd);
                    std::cout << "Setting volume to " << vol_val << ".\n";
                    if (status < 0)
                        std::cerr << "Error executing volume command: " << mpv_error_string(status) << std::endl;
                }
            }
            // FINAL HOLD: on end-of-file, hold last frame (do nothing).
            else if (command == "FINAL HOLD") {
                set_option(ctx, "keep-open", "yes");
            }
            // FINAL NOTHING: on end-of-file, just print "EOF".
            else if (command == "FINAL NOTHING") {
                set_option(ctx, "keep-open", "no");
            }
            // SETLOOPS ON: simply enable looping.
            else if (command == "SETLOOPS ON") {
                set_loops(ctx, true);
            }
            // SETLOOPS OFF: simply disable looping.
            else if (command == "SETLOOPS OFF") {
                set_loops(ctx, false);
            }
            // REMOVE or UNLOAD: unload (remove) current video.
            else if (command == "REMOVE" || command == "UNLOAD") {
                const char* unload_cmd[] = {"playlist-remove", "current", NULL};
                int status = mpv_command(ctx, unload_cmd);
                if (status < 0) {
                    std::cerr << "Error unloading current video: " << mpv_error_string(status) << std::endl;
                } else {
                    std::cout << "Unloaded current video (removed playlist entry).\n";
                }
            }
            else {
                std::cerr << "Unknown UDP command: \"" << command << "\"\n";
            }
        }
    }
    std::cout << "UDP Listener stopping." << std::endl;
    close(sockfd);
}

int main(int argc, char *argv[]) {
    // Default UDP port is 12345, but allow it to be overridden via command-line.
    int udp_port = 12345;
    if (argc >= 2) {
        udp_port = std::stoi(argv[1]);
    }
    mpv_handle *ctx = mpv_create();
    if (!ctx) {
        std::cerr << "Failed to create MPV context" << std::endl;
        return 1;
    }
    // Basic Options.
    set_option(ctx, "input-terminal", "no");
    set_option(ctx, "terminal", "no");
    set_option(ctx, "input-vo-keyboard", "yes");
    set_option(ctx, "input-default-bindings", "yes");
    // Window & Display.
    set_option(ctx, "force-window", "yes");
    set_option(ctx, "border", "no");
    set_option(ctx, "keep-open", "no"); // FINAL_HOLD behavior.
    set_option(ctx, "autofit", "500x500");
    set_option(ctx, "screen", "1");
    set_option(ctx, "window-dragging", "yes");
    // Playback & Seeking.
    set_option(ctx, "demuxer-max-bytes", "1GiB");
    set_option(ctx, "demuxer-max-back-bytes", "1GiB");
    set_option(ctx, "loop-file", "0");
    set_option(ctx, "hr-seek", "yes");
    set_option(ctx, "hr-seek-framedrop", "no");
    set_option(ctx, "resume-playback", "no");
    // Performance & Quality.
    set_option(ctx, "hwdec", "auto-safe");
    // Audio.
    set_option(ctx, "volume", "100");
    set_option(ctx, "volume-max", "100");
    // OSD.
    set_option(ctx, "osd-level", "0");

    int status = mpv_initialize(ctx);
    if (status < 0) {
        std::cerr << "Failed to initialize MPV: " << mpv_error_string(status) << std::endl;
        mpv_terminate_destroy(ctx);
        return 1;
    }
    print_controls();
    std::thread udp_thread(udp_listener, ctx, udp_port);
    udp_thread.detach();

    std::cout << "MPV Initialized. Waiting for events or quit signal...\n";

    // Main event loop.
    while (true) {
         mpv_event *event = mpv_wait_event(ctx, -1.0);
         if (event->event_id == MPV_EVENT_NONE)
            continue;
         if (event->event_id == MPV_EVENT_SHUTDOWN) {
            std::cout << "Received MPV shutdown event. Exiting." << std::endl;
            break;
         }
         if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
             auto msg = reinterpret_cast<mpv_event_log_message*>(event->data);
             std::cerr << "[mpv] " << msg->prefix << ": " << msg->level << ": " << msg->text;
             continue;
         }
         if (event->event_id == MPV_EVENT_END_FILE) {
             mpv_event_end_file* eef = (mpv_event_end_file*)event->data;
             if (eef->reason == MPV_END_FILE_REASON_ERROR && eef->error != 0) {
                  std::cerr << "MPV Error: Playback terminated with error: "
                            << mpv_error_string(eef->error) << std::endl;
             }
             else if (eef->reason == MPV_END_FILE_REASON_EOF) {
                  // Attract feature: if main video finished, play the attract video (if enabled).
                  if (use_attract && (current_video != attract_video)) {
                      std::cout << "Main video finished. Playing attract video." << std::endl;
                      set_loops(ctx, true);
                      load_file_command(ctx, attract_video, true);
                  } else {
                      std::cout << "Attract video finished or not used." << std::endl;
                  }
             }
         }
    }
    std::cout << "Terminating MPV..." << std::endl;
    mpv_terminate_destroy(ctx);
    std::cout << "MPV Terminated." << std::endl;
    return 0;
}
