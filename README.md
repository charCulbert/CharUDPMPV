# MPV UDP Controller

This project embeds the [mpv](https://mpv.io/) video player using **libmpv** and provides network control via UDP. It is designed for a static-IP environment where a controller sends commands (e.g. PLAY, LOAD, SEEK) directly to each player. The player, in turn, sends log/status messages (such as "READY") back to the controller.

## Overview

The system is modularized into three components:

- **UdpComm**  
  Handles all UDP communication—sending log/status messages to a controller on a designated port and listening for incoming commands on another port.

- **CommandProcessor**  
  Parses incoming command strings and maps them to corresponding MPV actions. Supported commands include:
    - `STATUS` – Replies with "READY"
    - `LOAD {FILENAME}` – Loads a file without changing playback state
    - `LOOPS {FILENAME}` – Loads a file with looping enabled
    - `PLAY {FILENAME}` – Loads a file with looping disabled and resumes playback
    - `PLAY` – Resumes playback if paused
    - `STOP` – Pauses playback
    - `SEEK <time>` – Seeks to the specified time in seconds
    - `VOL <number>` – Sets volume (0–100)
    - `FINAL HOLD` / `FINAL NOTHING` – Sets the keep-open option for end-of-file behavior
    - `SETLOOPS ON` / `SETLOOPS OFF` – Sets looping on/off without loading a file
    - `REMOVE` or `UNLOAD` – Unloads the current video
    - `ATTRACT {FILENAME}` – Sets the attract video
    - `USEATTRACT ON` / `USEATTRACT OFF` – Enables or disables the attract feature

- **main.cpp**  
  Handles configuration, MPV initialization, and overall orchestration. It reads settings from `player.conf`, sets up MPV options, creates a UdpComm instance, spawns a listener thread (which passes commands to the CommandProcessor), and processes MPV events.

## File Structure

- **UdpComm.h / UdpComm.cpp**  
  Contains the `UdpComm` class. This class opens a UDP socket on a configured listen port for incoming commands. It also provides the `sendLog()` method to send messages to the controller's static IP on a separate send port.

- **CommandProcessor.h / CommandProcessor.cpp**  
  Provides the `processCommand()` function. This function takes a command string (trimmed of whitespace), the MPV context, and a reference to the UdpComm instance, then parses the command and calls the appropriate MPV functions.

- **main.cpp**  
  The entry point for the application. It performs the following tasks:
    1. **Configuration:** Reads settings from `player.conf` (if present) to set UDP ports, controller IP, attract mode defaults, etc.
    2. **MPV Initialization:** Creates the MPV context and applies various player options.
    3. **UDP Communication Setup:** Creates a UdpComm instance.
    4. **Command Listener:** Spawns a separate thread that calls `UdpComm::runListener()`, passing each received command to `CommandProcessor::processCommand()`.
    5. **MPV Event Loop:** Processes MPV events (log messages, end-of-file events, shutdown events) and sends appropriate log messages back to the controller.

## Configuration

The application reads configuration from a file named `player.conf` located in the same directory. A sample configuration file:

```ini
# UDP port on which the player listens for commands.
udp_listen_port = 12345

# UDP port on which the player sends log/status messages.
udp_send_port = 12346

# Controller IP (the IP address of the system that sends commands).
controller_ip = 192.168.1.100

# Attract mode settings.
use_attract = true
default_loops = false
attract_video = attract.mp4

```
Any setting not found in the config file will default to the values defined in the source code.

## Building and Running
### Prerequisites
A C++ compiler with C++11 support or later.

libmpv installed on your system.

A POSIX-compatible environment (Linux, macOS, etc.).
