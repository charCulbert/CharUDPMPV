#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <string>
#include <netinet/in.h>
#include <mpv/client.h>
#include "UdpComm.h"

/**
 * @brief CommandProcessor maps command strings to MPV actions.
 *
 * This class encapsulates all command parsing and dispatch.
 */
class CommandProcessor {
public:
    /**
     * @brief Process a received command.
     * @param cmd The command string (already trimmed).
     * @param ctx The MPV handle.
     * @param udp The UdpComm instance for logging.
     * @param src The source address (for STATUS reply).
     * @param srcLen The length of the source address.
     */
    static void processCommand(const std::string &cmd, mpv_handle* ctx, UdpComm &udp,
                               const sockaddr_in &src, socklen_t srcLen);
};

#endif // COMMAND_PROCESSOR_H
