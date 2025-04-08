#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include "Player.h"
#include "Controller.h"
#include "json.hpp"

using json = nlohmann::json;

int main() {
    // --- Read configuration in main ---
    json config;
    std::ifstream configFile("player.json");
    if (!configFile) {
        std::cerr << "Failed to open player.json" << std::endl;
    } else {
        try {
            configFile >> config;
        } catch (json::parse_error &e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
    }

    // Create Player instance.
    Player player;

    // Update Player's configuration using the JSON keys.
    if (config.contains("controller_send_port"))
        player.udp_listen_port = config["controller_send_port"].get<int>();
    if (config.contains("controller_receive_port"))
        player.udp_send_port = config["controller_receive_port"].get<int>();
    if (config.contains("controller_ip"))
        player.controller_ip = config["controller_ip"].get<std::string>();
    if (config.contains("use_attract"))
        player.use_attract = config["use_attract"].get<bool>();
    if (config.contains("attract_video"))
        player.attract_video = config["attract_video"].get<std::string>();

    // Optionally, if you want to assign a name to the player:
    if (config.contains("player_name"))
        player.player_name = config["player_name"].get<std::string>();

    // Start the player in its own thread.
    std::thread playerThread(&Player::start, &player);
    playerThread.detach();

    // Check if this instance should act as a controller.
    bool is_controller = false;
    if (config.contains("is_controller"))
        is_controller = config["is_controller"].get<bool>();

    if (is_controller) {
        std::cout << "Operating as CONTROLLER" << std::endl;
        Controller controller;

        // Populate the controller's devices.
        if (config.contains("devices")) {
            // Assume "devices" is an object where each key is a device name,
            // and its value is an object that contains an "ip" field.
            for (auto &item : config["devices"].items()) {
                controller.devices[item.key()] = item.value()["ip"].get<std::string>();
            }
        }

        // Populate the controller's cues.
        if (config.contains("cues") && config["cues"].is_array()) {
            for (const auto &cue : config["cues"]) {
                controller.cues.push_back(cue);
            }
        }

        // Start the controller in its own thread.
        std::thread controllerThread(&Controller::start, &controller);
        controllerThread.detach();
    }

    // Keep main alive.
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
