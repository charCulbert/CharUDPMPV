#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include "Player.h"
#include "Controller.h"
#include "json.hpp"

using json = nlohmann::json;

int main()
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    // --- Read configuration in main ---
    json config;
    std::ifstream configFile("player.json");
    if (!configFile)
    {
        std::cerr << "Failed to open player.json" << std::endl;
    }
    else
    {
        try
        {
            configFile >> config;
        }
        catch (json::parse_error& e)
        {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
    }

    //////////////
    ////Player////
    //////////////

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
    /////////////////
    // CONTROLLER //
    ///////////////
    // Check if this instance should act as a controller.
    bool is_controller = false;
    if (config.contains("is_controller"))
        is_controller = config["is_controller"].get<bool>();

    // In main.cpp
    if (is_controller)
    {
        std::cout << "Operating as CONTROLLER" << std::endl;
        Controller controller;

        // Populate the controller's devices.
        if (config.contains("devices"))
        {
            std::cout << "Configuring devices:" << std::endl;
            for (auto& item : config["devices"].items())
            {
                std::string name = item.key();
                std::string ip = item.value()["ip"].get<std::string>();
                controller.devices[name] = ip;
                std::cout << "  Added device: " << name << " with IP: " << ip << std::endl;
            }
        }
        std::cout << "Total devices configured: " << controller.devices.size() << std::endl;

        // Populate the controller's cues.
        if (config.contains("cues") && config["cues"].is_array())
        {
            std::cout << "Loading " << config["cues"].size() << " cues" << std::endl;
            for (const auto& cue : config["cues"])
            {
                controller.cues.push_back(cue);
            }
        }

        // Start the controller - IMPORTANT: Use join() instead of detach()
        std::thread controllerThread(&Controller::start, &controller);
        controllerThread.join(); //
    }
    // Keep main alive.
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
