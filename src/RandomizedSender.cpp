// #include "RandomizedSender.h"
// #include <iostream>
// #include <cstdlib>
// #include <chrono>
// #include <thread>
//
// RandomizedSender::RandomizedSender(const std::string &deviceName, const std::unordered_map<std::string, std::string> &devices, UdpComm *udp)
//     : deviceName(deviceName), udp(udp), dots_on(false) {
//     // Look up the device IP from the devices map
//     if (devices.find(deviceName) != devices.end()) {
//         deviceIp = devices.at(deviceName);
//     } else {
//         std::cerr << "Device not found: " << deviceName << std::endl;
//     }
// }
//
// void RandomizedSender::setOnOff(bool state) {
//     std::lock_guard<std::mutex> lock(mutex);
//     dots_on = state;
// }
//
// void RandomizedSender::scheduleNext() {
//     // Sleep for a randomized amount of time (e.g., between 8 and 12 seconds)
//     std::this_thread::sleep_for(std::chrono::milliseconds(8000 + rand() % 4000));
//
//     // Generate random command (for example "PLAY a.mp4")
//     std::string command = generateRandomCommand();
//
//     // Check if dots_on is true before sending the command
//     {
//         std::lock_guard<std::mutex> lock(mutex);
//         if (!dots_on) {
//             return;  // If dots_on is false, don't send anything
//         }
//     }
//
//     // Send the generated command to the device
//     sendUdpMessage(command);
// }
//
// std::string RandomizedSender::generateRandomCommand() {
//     char randomChar = 'a' + rand() % 23;  // Random character between 'a' and 'w'
// return "PLAY " + "DOTS-" + std::string(1, randomChar) + ".mp4";
// }
//
// void RandomizedSender::sendUdpMessage(const std::string &command) {
//     std::cout << "Sending to " << deviceName << " (" << deviceIp << "): " << command << std::endl;
//     udp->sendUdpMessage(command, deviceIp, udp->getSendPort());  // Using the getter method for m_sendPort
// }
//
