#include <mpv/client.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <sstream>    // For parsing commands
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// A video player with libmpv so we can launch a video to loop seamlessly.

void set_option(mpv_handle* ctx, const char* name, const char* value) {
    int error = mpv_set_option_string(ctx, name, value);
    if (error < 0) {
        std::cout << "Failed to set option '" << name << "' to '" << value
                  << "': " << mpv_error_string(error) << std::endl;
        throw std::runtime_error(mpv_error_string(error));
    }
}

void print_controls() {
    std::cout << "\n=== MPV Player Controls ===\n"
              << "q             - Quit\n"
              << "Space         - Play/Pause\n"
              << "f             - Toggle fullscreen\n"
              << "1             - Jump to first frame\n"
              << "←/→           - Frame-step (when paused)\n"
              << "[Mouse]       - Click and drag to move window\n"
              << "[Mouse]       - Double click for fullscreen\n"
              << "UDP Commands:\n"
              << "   PLAY           - Resume playback\n"
              << "   STOP           - Pause playback\n"
              << "   SEEK <time>    - Seek to specified time (e.g., 'SEEK 0' for beginning)\n"
              << "   VOL <number>   - Set volume to the specified value\n"
              << "==========================================\n\n";
}

// UDP listener function running in its own thread.
// It listens for incoming UDP packets and processes commands.
void udp_listener(mpv_handle* ctx) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating UDP socket" << std::endl;
        return;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345); // UDP port
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding UDP socket" << std::endl;
        close(sockfd);
        return;
    }

    char buffer[1024];
    while (true) {
        ssize_t recv_len = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            std::string command(buffer);
            if (command.find("PLAY") != std::string::npos) {
                const char* cmd[] = {"set", "pause", "no", NULL};
                mpv_command(ctx, cmd);
                std::cout << "Received PLAY command. Resuming playback.\n";
            } else if (command.find("STOP") != std::string::npos) {
                const char* cmd[] = {"set", "pause", "yes", NULL};
                mpv_command(ctx, cmd);
                std::cout << "Received STOP command. Pausing playback.\n";
            } else if (command.rfind("SEEK", 0) == 0) { // Command starts with "SEEK"
                std::istringstream iss(command);
                std::string token;
                double time_val;
                if (!(iss >> token >> time_val)) {
                    std::cout << "Invalid SEEK command format. Use: SEEK <time>\n";
                } else {
                    std::string timeStr = std::to_string(time_val);
                    const char* cmd[] = {"seek", timeStr.c_str(), "absolute", NULL};
                    mpv_command(ctx, cmd);
                    std::cout << "Received SEEK command. Seeking to " << time_val << " seconds.\n";
                }
            } else if (command.rfind("VOL", 0) == 0) { // Command starts with "VOL"
                std::istringstream iss(command);
                std::string token;
                int vol_val;
                if (!(iss >> token >> vol_val)) {
                    std::cout << "Invalid VOL command format. Use: VOL <number>\n";
                } else {
                    std::string volStr = std::to_string(vol_val);
                    const char* cmd[] = {"set", "volume", volStr.c_str(), NULL};
                    mpv_command(ctx, cmd);
                    std::cout << "Received VOL command. Setting volume to " << vol_val << ".\n";
                }
            }
        }
    }
    close(sockfd);
}

int main(int argc, char *argv[]) {
    mpv_handle *ctx = mpv_create();
    if (!ctx) {
        std::cout << "Failed to create MPV context" << std::endl;
        return 1;
    }

    try {
        // Basic options
        set_option(ctx, "force-window", "yes");
        set_option(ctx, "terminal", "yes");
        set_option(ctx, "keep-open", "yes");
        set_option(ctx, "border", "no");
        set_option(ctx, "input-default-bindings", "yes");
        set_option(ctx, "input-vo-keyboard", "yes");

        // Frame-perfect seeking options
        set_option(ctx, "hr-seek", "yes");
        set_option(ctx, "hr-seek-framedrop", "no");
        set_option(ctx, "resume-playback", "no");
        set_option(ctx, "cache", "yes");
        set_option(ctx, "cache-secs", "10");
        set_option(ctx, "demuxer-seekable-cache", "yes");

        // Video output options for better frame accuracy
        set_option(ctx, "video-sync", "display-resample");
        set_option(ctx, "interpolation", "no");

        // Set volume and disable OSDs
        set_option(ctx, "volume", "100");
        set_option(ctx, "volume-max", "100");
        set_option(ctx, "osd-level", "0");
        set_option(ctx, "osd-bar", "no");
        set_option(ctx, "osd-on-seek", "no");

        // Set default window size and looping options
        set_option(ctx, "autofit", "500x500");
        set_option(ctx, "screen", "1");
        set_option(ctx, "loop", "inf");
        set_option(ctx, "window-dragging", "yes");

        // Initialize MPV
        int status = mpv_initialize(ctx);
        if (status < 0) {
            std::cout << "Failed to initialize MPV: " << mpv_error_string(status) << std::endl;
            mpv_destroy(ctx);
            return 1;
        }

        print_controls();

        // Start the UDP listener thread
        std::thread udp_thread(udp_listener, ctx);
        udp_thread.detach();

        // Load video
        const char* cmd[] = {"loadfile", "../loop.mp4", NULL};
        status = mpv_command(ctx, cmd);
        if (status < 0) {
            std::cout << "Failed to load file: " << mpv_error_string(status) << std::endl;
            mpv_destroy(ctx);
            return 1;
        }

        std::cout << "Starting playback in fullscreen mode...\n";

        // Main event loop for MPV
        while (true) {
            mpv_event* event = mpv_wait_event(ctx, -1);
            if (event->event_id == MPV_EVENT_SHUTDOWN)
                break;
            if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
                auto msg = reinterpret_cast<mpv_event_log_message*>(event->data);
                std::cout << "[" << msg->prefix << "] " << msg->text;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        mpv_destroy(ctx);
        return 1;
    }

    mpv_destroy(ctx);
    return 0;
}
