#include "io_utils.h"

#include <cerrno>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>

#include <fcntl.h>
#include <sys/socket.h>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// 仅用于测试的本地连接对。
struct LocalPair {
    net::UniqueFd sender;
    net::UniqueFd receiver;

    LocalPair() {
        int fds[2] {-1, -1};

        if (::socketpair(AF_UNIX, SOCK_STREAM, 0 ,fds) == -1) {
            const int errorCode = errno;

            throw std::system_error(
                errorCode,
                std::generic_category(),
                "socketpair failed"
            );
        }

        // 临时 UniqueFd的资源通过移动赋值转交给成员
        sender = net::UniqueFd{fds[0]};
        receiver = net::UniqueFd{fds[1]};
    }
};

void RequireClosed(int fd) {
    errno = 0;

    const int result = ::fcntl(fd, F_GETFD);
    const int errorCode = errno;

    Require(result == -1, "Descriptor must be closed");
    Require(errorCode == EBADF, "Expected EBADF");
}

void TestDefaultAndDestructor() {
    net::UniqueFd empty;

    Require(!empty.IsValid(), "Default object must be empty");
    Require(empty.Get() == -1, "Expected invalid descriptor");

    empty.Close();
    empty.Close();

    int saveFd = -1;
    {
        LocalPair pair;
        saveFd = pair.receiver.Get();
    }

    RequireClosed(saveFd);
}

void TestMoveConstructor() {
    LocalPair pair;
    const int saveFd = pair.receiver.Get();
    {
        net::UniqueFd owner{std::move(pair.receiver)};

        Require(!pair.receiver.IsValid(), "Source must be empty");
        Require(owner.Get() == saveFd, "Descriptor must transfer");

        // 移动后的原对象不能再关闭已经转交的描述符.
        pair.receiver.Close();
        Require(::fcntl(owner.Get(), F_GETFD) != -1, "Transferred descriptor must remain open");
    }

    RequireClosed(saveFd);
}

void TestMoveAssignment() {
    LocalPair first;
    LocalPair second;
    const int transferredFd = first.receiver.Get();
    const int replacedFd = second.receiver.Get();

    second.receiver = std::move(first.receiver);

    Require(!first.receiver.IsValid(), "Source must be empty");
    Require(second.receiver.Get() == transferredFd, "Assignment must transfer descriptor");

    // 移动赋值必须释放目标对象原来拥有的资源
    RequireClosed(replacedFd);

    second.receiver.Close();
    RequireClosed(transferredFd);

    second.receiver.Close();
}

void TestNonBlockingFlag() {
    LocalPair pair;
    const int before = ::fcntl(pair.receiver.Get(), F_GETFL);
    Require(before != -1, "Initial F_GETFL failed");

    net::SetNonBlocking(pair.receiver.Get());
    net::SetNonBlocking(pair.receiver.Get());

    const int after = ::fcntl(pair.receiver.Get(), F_GETFL);
    Require(after != -1, "Final F_GETFL failed");
    Require((after & O_NONBLOCK) != 0, "O_NONBLOCK must be enabled");

    Require(
        (after & ~O_NONBLOCK) == (before & ~O_NONBLOCK),
        "Other status flags must be preserved"
    );
}

void TestReadOutcomes() {
    LocalPair pair;
    net::SetNonBlocking(pair.receiver.Get());

    const auto empty = net::TryReceive(pair.receiver.Get());

    Require(
        empty.status == net::ReadStatus::kWouldBlock, "Empty socket must report WOULD_BLOCK"
    );
    Require(empty.data.empty(), "No data expected");
    Require(
        ::fcntl(pair.receiver.Get(), F_GETFD) != -1,
        "WOULD_BLOCK must not close the socket"
    );

    // 用一个零字节验证：零字节数据不等于EOF
    const char byte = '\0';
    Require(
        ::send(pair.sender.Get(), &byte, 1, MSG_NOSIGNAL) == 1,
        "Sending one byte failed"
    );

    const auto received = net::TryReceive(pair.receiver.Get());

    Require(
        received.status == net::ReadStatus::kData,
        "Expected DATA"
    );
    Require(received.data.size() == 1, "Expected one byte");
    Require(received.data[0] == '\0', "Expected a zero byte");

    const auto drained = net::TryReceive(pair.receiver.Get());
    Require(
        drained.status == net::ReadStatus::kWouldBlock,
        "Drained socket must report WOULD_BLOCK"
    );

    Require(
        ::shutdown(pair.sender.Get(), SHUT_WR) == 0,
        "shutdown failed"
    );

    const auto ended = net::TryReceive(pair.receiver.Get());

    Require(
        ended.status == net::ReadStatus::kPeerClosed,
        "Expected PEER_CLOSED"
    );

    // TryReceive 只报告结果，不负责关闭资源。
    Require(
        pair.receiver.IsValid(),
        "Owner still holds the descriptor"
    );

}

void TestInvalidDescriptor() {
    bool setFailed = false;

    try {
        net::SetNonBlocking(-1);
    } catch (const std::system_error& error) {
        if (error.code().value() != EBADF) {
            throw;
        }
        setFailed = true;
    }

    Require(setFailed, "Invalid descriptor must be rejected");

    bool readFailed = false;

    try {
        (void)net::TryReceive(-1);
    } catch (const std::system_error& error) {
        if (error.code().value() != EBADF) {
            throw;
        }
        readFailed = true;
    }

    Require(readFailed, "Invalid receive must fail");
}

}

static_assert(!std::is_copy_constructible_v<net::UniqueFd>);
static_assert(!std::is_copy_assignable_v<net::UniqueFd>);
static_assert(std::is_nothrow_move_constructible_v<net::UniqueFd>);
static_assert(std::is_nothrow_move_assignable_v<net::UniqueFd>);

int main() {
    try {
        TestDefaultAndDestructor();
        TestMoveConstructor();
        TestMoveAssignment();
        TestNonBlockingFlag();
        TestReadOutcomes();
        TestInvalidDescriptor();

        std::cout << "All IO utility tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}