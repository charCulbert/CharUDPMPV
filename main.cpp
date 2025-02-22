#include <mpv/client.h>
#include <iostream>
#include <stdexcept>
#include <string>

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
              << "q     - Quit\n"
              << "Space - Play/Pause\n"
              << "f     - Toggle fullscreen\n"
              << "1     - Jump to first frame\n"
              << "←/→   - Frame-step (when paused)\n"
              << "[Mouse] Click and drag to move window\n"
              << "[Mouse] Double click for fullscreen\n"
              << "==========================================\n\n";
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
        set_option(ctx, "hr-seek", "yes");          // Enable precise seeking
        set_option(ctx, "hr-seek-framedrop", "no"); // Never drop frames during seek
        set_option(ctx, "resume-playback", "no");   // Don't auto-resume after seek
        set_option(ctx, "cache", "yes");            // Enable caching
        set_option(ctx, "cache-secs", "10");        // Cache 10 seconds
        set_option(ctx, "demuxer-seekable-cache", "yes"); // Enable seekable cache

        // Video output options for better frame accuracy
        set_option(ctx, "video-sync", "display-resample"); // Precise frame timing
        set_option(ctx, "interpolation", "no");     // Disable interpolation

        // Set volume to 100 and disable controls
        set_option(ctx, "volume", "100");
        set_option(ctx, "volume-max", "100");

        // Disable OSDs
        set_option(ctx, "osd-level", "0");          // Disable OSD completely
        set_option(ctx, "osd-bar", "no");           // Disable seek bar
        set_option(ctx, "osd-on-seek", "no");       // Disable OSD on seek


        // Set a reasonable default window size (e.g., 720p)
        set_option(ctx, "autofit", "500x500");

        // // Window settings
        // set_option(ctx, "fullscreen", "yes");
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

        // Load video
        const char* cmd[] = {"loadfile", "../looping1.mp4", NULL};
        status = mpv_command(ctx, cmd);
        if (status < 0) {
            std::cout << "Failed to load file: " << mpv_error_string(status) << std::endl;
            mpv_destroy(ctx);
            return 1;
        }

        std::cout << "Starting playback in fullscreen mode...\n";

        // Event loop
        while (1) {
            mpv_event* event = mpv_wait_event(ctx, -1);

            if (event->event_id == MPV_EVENT_SHUTDOWN)
                break;

            if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
                mpv_event_log_message* msg = (mpv_event_log_message*)event->data;
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