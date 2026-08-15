#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <utility>

#define ASIO_NO_TYPEID 1
#include <asio/awaitable.hpp>

#include "ws_client/BufferedSocketAsync.hpp"
#include "ws_client/config.hpp"
#include "ws_client/WebSocketClient.hpp"
#include "ws_client/WebSocketClientAsync.hpp"
#include "ws_client/log.hpp"
#include "ws_client/transport/builtin/OpenSslSocket.hpp"

namespace
{
using ws_client::WSError;
using ws_client::byte;

struct MoveOnlySocket
{
    MoveOnlySocket() = default;
    MoveOnlySocket(MoveOnlySocket&&) noexcept = default;
    MoveOnlySocket& operator=(MoveOnlySocket&&) noexcept = default;
    MoveOnlySocket(const MoveOnlySocket&) = delete;
    MoveOnlySocket& operator=(const MoveOnlySocket&) = delete;

    std::expected<bool, WSError> wait_readable(ws_client::Timeout<>&) noexcept;
    std::expected<std::size_t, WSError> read_some(std::span<byte>, ws_client::Timeout<>&) noexcept;
    std::expected<std::size_t, WSError>
    write_some(std::span<const byte>, ws_client::Timeout<>&) noexcept;
    std::expected<void, WSError> shutdown(bool, ws_client::Timeout<>&) noexcept;
    std::expected<void, WSError> close(bool) noexcept;
};

struct MoveOnlyMaskKeyGen
{
    MoveOnlyMaskKeyGen() = default;
    MoveOnlyMaskKeyGen(MoveOnlyMaskKeyGen&&) noexcept = default;
    MoveOnlyMaskKeyGen& operator=(MoveOnlyMaskKeyGen&&) noexcept = default;
    MoveOnlyMaskKeyGen(const MoveOnlyMaskKeyGen&) = delete;
    MoveOnlyMaskKeyGen& operator=(const MoveOnlyMaskKeyGen&) = delete;

    ws_client::MaskKey operator()() noexcept
    {
        return ws_client::MaskKey(1);
    }
};

struct MoveOnlyAsyncSocket
{
    MoveOnlyAsyncSocket() = default;
    MoveOnlyAsyncSocket(MoveOnlyAsyncSocket&&) noexcept = default;
    MoveOnlyAsyncSocket& operator=(MoveOnlyAsyncSocket&&) noexcept = default;
    MoveOnlyAsyncSocket(const MoveOnlyAsyncSocket&) = delete;
    MoveOnlyAsyncSocket& operator=(const MoveOnlyAsyncSocket&) = delete;

    std::expected<bool, WSError> can_read() noexcept;
    asio::awaitable<std::expected<std::size_t, WSError>>
    read_some(std::span<byte>, ws_client::Timeout<>&) noexcept;
    asio::awaitable<std::expected<std::size_t, WSError>>
    write_some(std::span<const byte> buffer, ws_client::Timeout<>&) noexcept
    {
        co_return buffer.size();
    }
    asio::awaitable<std::expected<void, WSError>> shutdown(bool, ws_client::Timeout<>&) noexcept;
    asio::awaitable<std::expected<void, WSError>> close(bool) noexcept;
};

using SyncClient =
    ws_client::WebSocketClient<ws_client::ConsoleLogger, MoveOnlySocket, MoveOnlyMaskKeyGen>;
using AsyncClient = ws_client::WebSocketClientAsync<
    asio::awaitable,
    ws_client::ConsoleLogger,
    MoveOnlyAsyncSocket,
    MoveOnlyMaskKeyGen>;
using AsyncBufferedSocket =
    ws_client::BufferedSocketAsync<MoveOnlyAsyncSocket, asio::awaitable>;

void instantiate_sync_client_moves(ws_client::ConsoleLogger* logger)
{
    SyncClient source(logger, MoveOnlySocket{}, MoveOnlyMaskKeyGen{});
    SyncClient moved(std::move(source));
    SyncClient assigned(logger, MoveOnlySocket{}, MoveOnlyMaskKeyGen{});
    assigned = std::move(moved);
}

void instantiate_async_client_moves(ws_client::ConsoleLogger* logger)
{
    AsyncClient source(logger, MoveOnlyAsyncSocket{}, MoveOnlyMaskKeyGen{});
    AsyncClient moved(std::move(source));
    AsyncClient assigned(logger, MoveOnlyAsyncSocket{}, MoveOnlyMaskKeyGen{});
    assigned = std::move(moved);
}

asio::awaitable<void> instantiate_async_buffered_write(
    AsyncBufferedSocket& socket,
    std::span<const byte> buffer,
    ws_client::Timeout<>& timeout
)
{
    auto result = co_await socket.write_some(buffer, timeout);
    (void)result;
}

void instantiate_openssl_accessors(const ws_client::OpenSslSocket<ws_client::ConsoleLogger>& socket)
{
    std::expected<std::string, WSError> cipher = socket.get_current_cipher();
    std::expected<int, WSError> version = socket.get_current_tls_version();
    (void)cipher;
    (void)version;
}
} // namespace
