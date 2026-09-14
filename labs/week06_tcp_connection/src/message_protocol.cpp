#include "message_protocol.h"

#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>

namespace protocol {
    void SendMessage(TcpSocket& socket, std::string_view message) {
        if (message.size() > kMaxMessageSize) {
            throw std::length_error("message too long");
        }

        const auto bodyLength = static_cast<std::uint32_t>(message.size());
        const std::uint32_t networkLength = htonl(bodyLength);
        const std::string_view header(
            reinterpret_cast<const char*>(&networkLength),
            sizeof(networkLength)
        );

        socket.SendAll(header);
        socket.SendAll(message);
    }

    std::string ReceiveMessage(TcpSocket& socket) {
        try {
            const std::string header = socket.ReceiveExact(sizeof(std::uint32_t));

            std::uint32_t networkLength = 0;

            std::memcpy(
                &networkLength,
                header.data(),
                sizeof(networkLength)
            );

            const std::uint32_t bodyLength = ntohl(networkLength);

            if (bodyLength > protocol::kMaxMessageSize) {
                throw std::length_error("message too long");
            }

            return socket.ReceiveExact(bodyLength);

        } catch(...) {
            socket.Abort();
            throw;
        }
        
    }
}

