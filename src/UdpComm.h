#ifndef UDP_COMM_H
#define UDP_COMM_H

#include <string>
#include <netinet/in.h>
#include <functional>

/**
 * @brief UdpComm handles UDP communication.
 *
 * This class encapsulates sending log/status messages to the controller and
 * listening for incoming commands on a given port.
 */
class UdpComm {
public:
    /**
     * @brief Construct a new UdpComm object.
     * @param listenPort Port on which to listen for commands.
     * @param sendPort Port on which to send log/status messages.
     * @param controllerIp Controller's static IP address.
     */
    UdpComm(int listenPort, int sendPort, const std::string &controllerIp);

    ~UdpComm();
    UdpComm(const UdpComm&) = delete;
    UdpComm& operator=(const UdpComm&) = delete;

    /**
     * @brief Send a log or status message to the controller.
     * @param msg The message to send.
     */
    void sendLog(const std::string &msg);

    /**
     * @brief Run the UDP listener.
     *
     * For every command received on the listen port, the handler callback is invoked.
     *
     * @param handler A function that processes a received command (after trimming)
     *                along with the sender's address information.
     */
    void runListener(const std::function<void(const std::string&, const struct sockaddr_in&, socklen_t)>& handler);

private:
    int m_listenPort;
    int m_sendPort;
    std::string m_controllerIp;
};

#endif // UDP_COMM_H
