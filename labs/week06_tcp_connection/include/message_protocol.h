#pragma once

#include "tcp_socket.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace protocol {
    inline constexpr std::uint32_t kMaxMessageSize = 1024;

    void SendMessage(TcpSocket& socket, std::string_view message);
    std::string ReceiveMessage(TcpSocket& socket);
    
}