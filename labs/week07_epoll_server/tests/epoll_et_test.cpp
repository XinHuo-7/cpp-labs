#include "epoll_poller.h"
#include "io_utils.h"

#include <cerrno>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <sys/socket.h>

namespace {
    
void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct LocalPair {
    net::UniqueFd sender;
    net::UniqueFd receiver;

    LocalPair() {
        int fds[2]{-1, -1};

        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
            const int errorCode = errno;

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "socketpair failed"
            );
        }

        sender = net::UniqueFd{fds[0]};
        receiver = net::UniqueFd{fds[1]};

        // 接收端必须非阻塞，否则读空后会阻塞在下一次 recv。
        net::SetNonBlocking(receiver.Get());

        // 测试发送端也设为非阻塞，避免意外阻塞测试线程。
        net::SetNonBlocking(sender.Get());
    }
};

void SendTestData(int fd, std::string_view data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t count = ::send(
            fd,
            data.data() + sent,
            data.size() - sent,
            MSG_NOSIGNAL
        );

        if (count == -1) {
            const int errorCode = errno;

            if (errorCode == EINTR) {continue;}
                        // 本测试只发送少量本地数据。
            // 如果发生 EAGAIN，直接使测试失败，不进行忙等。
            // 完整的非阻塞发送队列留到后续实现。
            throw std::system_error(
                errorCode,
                std::generic_category(),
                "test send failed"
            );
        }
        Require(count > 0, "Send made no progress");
        sent += static_cast<std::size_t>(count);
    }
}

bool HasReadable(const std::vector<epoll_event>& events, int fd) {
    for (const auto& event : events) {
        if (event.data.fd == fd && (event.events & EPOLLIN) != 0) {
            return true;
        }
    }
    return false;
}

char ReadOneByte(int fd) {
    char value{};

    for (;;) {
        const ssize_t count = recv(fd, &value, 1, 0);
        if (count == -1) {
            const int errorCode = errno;

            if (errorCode == EINTR) {
                continue;
            }

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "reading one byte failed"
            );
        }

        Require(count == 1, "Expected exactly one byte");
        return value;
    }
}

// 使用同一套操作，对照 LT 和 ET。
// 测试过程中没有其他读取者，也没有新的发送或注册变化。
void TestPartialRead(bool edgeTriggered) {
    LocalPair pair;
    net::EpollPoller poller;

    std::uint32_t interests = EPOLLIN;
    if (edgeTriggered) {
         // |=：在原位掩码中增加 EPOLLET。
         interests |= EPOLLET;
    }
    
    poller.Add(pair.receiver.Get(), interests);
    SendTestData(pair.sender.Get(), "AB");

    Require(HasReadable(poller.Wait(1000), pair.receiver.Get()), "Expected initial readable event");

    // 故意只读取 A，把 B 留在接收缓冲区。
    Require(ReadOneByte(pair.receiver.Get()) == 'A', "Expected A");

    const auto repeated = poller.Wait(0);
    const bool notifiedAgain = HasReadable(repeated, pair.receiver.Get());

    std::cout << (edgeTriggered ? "[ET]" : "[LT]")
              << "只读 A 后再次通知: "
              << (notifiedAgain ? "是" : "否")
              << '\n';
    
    if (edgeTriggered) {
        Require(repeated.empty(), "No repeated event expected in this controlled ET case");
    } else {
        Require(notifiedAgain, "LT must still report unread data");
    }

    // 关键：即使 ET 没有再次通知，B 仍然真实存在。
    // 我们主动继续读取，而不是等待一个不保证到来的新通知。
    const auto remaining = net::DrainReadable(pair.receiver.Get());
    Require(remaining.data == "B", "Expected remaining B");
    Require(!remaining.peerClosed, "Peer is still open");

    std::cout << "主动继续读取: " << remaining.data << '\n';

    // 已经读取到 EAGAIN，此后新数据到达应能再次得到通知。
    SendTestData(pair.sender.Get(), "C");

    Require(HasReadable(poller.Wait(1000), pair.receiver.Get()), "New data must be observable");

    const auto next = net::DrainReadable(pair.receiver.Get());
    Require(next.data == "C", "Expected new data C");
    Require(!next.peerClosed, "Peer is still open");
}

// 验证 DrainReadable 不仅调用一次 TryReceive。
void TestDrainMultipleChunks() {
    LocalPair pair;
    net::EpollPoller poller;

    poller.Add(pair.receiver.Get(), EPOLLIN | EPOLLET);

    // TryReceive 单次缓冲区是 4096 字节。
    // 8193 字节要求至少三次成功读取，之后还要读到 EAGAIN。
    const std::string payload(8193, 'X');
    SendTestData(pair.sender.Get(), payload);
    Require(
        HasReadable(poller.Wait(1000), pair.receiver.Get()),
        "Expected readable event"
    );
    const auto result = net::DrainReadable(pair.receiver.Get());
    Require(result.data == payload, "Drain lost data");
    Require(!result.peerClosed, "Unexpected EOF");
    const auto empty = net::TryReceive(pair.receiver.Get());
    Require(
        empty.status == net::ReadStatus::kWouldBlock,
        "Socket must be drained to EAGAIN"
    );
    Require(poller.Wait(0).empty(), "No unread data expected");
    std::cout << "[drain] 累计读取字节数："
              << result.data.size() << '\n';

}

void TestDataAndEofTogether() {
    LocalPair pair;
    net::EpollPoller poller;

    poller.Add(pair.receiver.Get(), EPOLLIN | EPOLLET);

    const std::string payload("A\0B", 3);
    SendTestData(pair.sender.Get(), payload);

    Require(
        ::shutdown(pair.sender.Get(), SHUT_WR) == 0,
        "shutdown failed"
    );

    Require(
        HasReadable(poller.Wait(1000), pair.receiver.Get()),
        "Expected readable event"
    );

    const auto result = net::DrainReadable(pair.receiver.Get());

    // 同一次持续读取，既取得剩余数据，也观察到 EOF。
    Require(result.data == payload, "Data before EOF was lost");
    Require(result.peerClosed, "Expected EOF");

        std::cout
        << "[EOF] 已读字节数：" << result.data.size()
        << ", peerClosed=true\n";
    // 当前策略：处理完结果后，取消关注并关闭本端。
    poller.Remove(pair.receiver.Get());
    pair.receiver.Close();
}

void TestEmptyAndLimits() {
    LocalPair pair;

    const auto empty = net::DrainReadable(pair.receiver.Get());

    Require(empty.data.empty(), "Expected no data");
    Require(!empty.peerClosed, "No data is not EOF");

    bool invalidLimit = false;

    try {
        (void)net::DrainReadable(pair.receiver.Get(), 0);
    } catch (const std::invalid_argument&) {
        invalidLimit = true;
    }
    Require(invalidLimit, "Zero limit must be rejected");

    SendTestData(pair.sender.Get(), "ABCDE");

    bool exceeded = false;

    try {
        (void)net::DrainReadable(pair.receiver.Get(), 4);
    } catch (const std::length_error&) {
        exceeded = true;
    }
    Require(exceeded, "Receive limit must be enforced");

    // 超限前可能已消耗字节，不再尝试恢复或复用。
    pair.receiver.Close();

}

void TestInvalidDescriptor() {
    bool caught = false;

    try {
        (void)net::DrainReadable(-1);
    } catch (const std::system_error& error) {
        if (error.code().value() != EBADF) {
            throw;
        }

        caught = true;
    }

    Require(caught, "Unexpected errors must propagate"); 
}


} // namespace

int main() {
    try {
        TestPartialRead(false); // LT 对照
        TestPartialRead(true);  // ET 对照
        TestDrainMultipleChunks();
        TestDataAndEofTogether();
        TestEmptyAndLimits();
        TestInvalidDescriptor();

        std::cout << "All ET tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}