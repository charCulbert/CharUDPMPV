#include "UdpComm.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cctype>

UdpComm::UdpComm(int listenPort, int sendPort, const std::string &controllerIp)
    : m_listenPort(listenPort), m_sendPort(sendPort), m_controllerIp(controllerIp)
{
}

UdpComm::~UdpComm() {
    // No persistent socket to close.
}

void UdpComm::sendLog(const std::string &msg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "UdpComm: Failed to create socket for sending logs: " << strerror(errno) << std::endl;
        return;
    }

    // Enable broadcast.
    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        std::cerr << "UdpComm: Failed to set SO_BROADCAST: " << strerror(errno) << std::endl;
        close(sock);
        return;
    }

    sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(m_sendPort);
    destAddr.sin_addr.s_addr = inet_addr(m_controllerIp.c_str());

    ssize_t sent = sendto(sock, msg.c_str(), msg.size(), 0,
                          reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
    if (sent < 0)
        std::cerr << "UdpComm: Error sending log: " << strerror(errno) << std::endl;
    close(sock);
}

void UdpComm::sendUdpMessage(const std::string &msg, const std::string &destIp, int destPort) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "UdpComm: Failed to create socket for sending UDP message: " << strerror(errno) << std::endl;
        return;
    }
    // Enable broadcast in case we're using a broadcast address.
    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        std::cerr << "UdpComm: Failed to set SO_BROADCAST: " << strerror(errno) << std::endl;
        close(sock);
        return;
    }

    sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(destPort);
    destAddr.sin_addr.s_addr = inet_addr(destIp.c_str());

    ssize_t sent = sendto(sock, msg.c_str(), msg.size(), 0,
                          reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
    if (sent < 0)
        std::cerr << "UdpComm: Error sending UDP message: " << strerror(errno) << std::endl;
    close(sock);
}

void UdpComm::runListener(const std::function<void(const std::string&, const sockaddr_in&, socklen_t)>& handler) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        sendLog("UdpComm: Error creating UDP socket: " + std::string(strerror(errno)));
        return;
    }
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        sendLog("UdpComm: Warning: setsockopt(SO_REUSEADDR) failed: " + std::string(strerror(errno)));
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_listenPort);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        sendLog("UdpComm: Error binding UDP socket to port " + std::to_string(m_listenPort) + ": " + strerror(errno));
        close(sockfd);
        return;
    }
    // sendLog("UdpComm: UDP Listener started on port " + std::to_string(m_listenPort) + ".");

    char buffer[1024];
    while (true) {
        sockaddr_in src;
        socklen_t srcLen = sizeof(src);
        ssize_t bytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                 reinterpret_cast<sockaddr*>(&src), &srcLen);
        if (bytes < 0) {
            sendLog("UdpComm: Error receiving UDP data: " + std::string(strerror(errno)));
            usleep(100000); // Sleep 100ms on error.
            continue;
        }
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string command(buffer);
            // Trim trailing newline/carriage returns.
            while (!command.empty() && (command.back() == '\n' || command.back() == '\r'))
                command.pop_back();
            // sendLog("UdpComm: Received command: \"" + command + "\"");
            handler(command, src, srcLen);
        }
    }
    // This code is unreachable in the current infinite loop design.
    sendLog("UdpComm: UDP Listener stopping.");
    close(sockfd);
}
