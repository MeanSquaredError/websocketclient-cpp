#include <cstddef>
#include <expected>
#include <span>

#include <ws_client/ws_client.hpp>

class FakeSocket
{
public:
    std::expected<std::size_t, ws_client::WSError>
    read_some(std::span<std::byte>, ws_client::Timeout<>&) noexcept
    {
        return 0;
    }

    std::expected<std::size_t, ws_client::WSError>
    write_some(std::span<std::byte> buffer, ws_client::Timeout<>&) noexcept
    {
        return buffer.size();
    }

    std::expected<bool, ws_client::WSError> wait_readable(ws_client::Timeout<>&) noexcept
    {
        return false;
    }

    std::expected<void, ws_client::WSError> shutdown(bool, ws_client::Timeout<>&) noexcept
    {
        return {};
    }

    std::expected<void, ws_client::WSError> close(bool) noexcept
    {
        return {};
    }
};

int main()
{
    ws_client::ConsoleLogger logger(ws_client::LogLevel::N);
    ws_client::WebSocketClient<ws_client::ConsoleLogger, FakeSocket> client(&logger, FakeSocket{});

    return client.is_closed() ? 0 : 1;
}
