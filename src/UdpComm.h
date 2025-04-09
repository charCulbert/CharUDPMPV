#ifndef UDP_COMM_H
#define UDP_COMM_H

#include <string>
#include <functional>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
class UdpComm {
public:
    // Constructor and destructor.
    UdpComm(int listenPort, int sendPort, const std::string &controllerIp);
    ~UdpComm();

    // Sends a log message (prints to stdout and sends via UDP to the controller broadcast address).
    void sendLog(const std::string &msg);

    // Sends a UDP message to the specified destination IP and port.
    void sendUdpMessage(const std::string &msg, const std::string &destIp, int destPort);

    // Runs the UDP listener, calling the provided callback for each received message.
    // The callback receives the received message, the source sockaddr_in and its length.
    void runListener(const std::function<void(const std::string&, const struct sockaddr_in&, socklen_t)>& handler);

private:
    int m_listenPort;
    int m_sendPort;
    std::string m_controllerIp;
};

#endif // UDP_COMM_H
