#include "UdpComm.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
typedef int socklen_t;
typedef long ssize_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#endif

#include <cctype>
#include <chrono>
#include <thread>

UdpComm::UdpComm(int listenPort, int sendPort, const std::string &controllerIp)
    : m_listenPort(listenPort), m_sendPort(sendPort), m_controllerIp(controllerIp)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
    }
#endif
}

UdpComm::~UdpComm() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void UdpComm::sendLog(const std::string &msg) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "UdpComm: Failed to create socket for sending logs: " << strerror(errno) << std::endl;
        return;
    }

    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                   reinterpret_cast<const char*>(&broadcastEnable), sizeof(broadcastEnable)) < 0) {
        std::cerr << "UdpComm: Failed to set SO_BROADCAST: " << strerror(errno) << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return;
    }

    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(m_sendPort);
    destAddr.sin_addr.s_addr = inet_addr(m_controllerIp.c_str());

    ssize_t sent = sendto(sock, msg.c_str(), msg.size(), 0,
                          reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
    if (sent < 0)
        std::cerr << "UdpComm: Error sending log: " << strerror(errno) << std::endl;

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

void UdpComm::sendUdpMessage(const std::string &msg, const std::string &destIp, int destPort) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "UdpComm: Failed to create socket for sending UDP message: " << strerror(errno) << std::endl;
        return;
    }

    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                   reinterpret_cast<const char*>(&broadcastEnable), sizeof(broadcastEnable)) < 0) {
        std::cerr << "UdpComm: Failed to set SO_BROADCAST: " << strerror(errno) << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return;
    }

    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(destPort);
    destAddr.sin_addr.s_addr = inet_addr(destIp.c_str());

    ssize_t sent = sendto(sock, msg.c_str(), msg.size(), 0,
                          reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
    if (sent < 0)
        std::cerr << "UdpComm: Error sending UDP message: " << strerror(errno) << std::endl;

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

void UdpComm::runListener(const std::function<void(const std::string&, const sockaddr_in&, socklen_t)>& handler) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        sendLog("UdpComm: Error creating UDP socket: " + std::string(strerror(errno)));
        return;
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse)) < 0) {
        sendLog("UdpComm: Warning: setsockopt(SO_REUSEADDR) failed: " + std::string(strerror(errno)));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_listenPort);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        sendLog("UdpComm: Error binding UDP socket to port " + std::to_string(m_listenPort) + ": " + strerror(errno));
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return;
    }

    char buffer[1024];
    while (true) {
        sockaddr_in src{};
        socklen_t srcLen = sizeof(src);
        ssize_t bytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                 reinterpret_cast<sockaddr*>(&src), &srcLen);
        if (bytes < 0) {
            sendLog("UdpComm: Error receiving UDP data: " + std::string(strerror(errno)));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string command(buffer);
            while (!command.empty() && (command.back() == '\n' || command.back() == '\r'))
                command.pop_back();
            handler(command, src, srcLen);
        }
    }

#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}
