#include <gtest/gtest.h>

#include <array>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "ws_client/config.hpp"
#include "ws_client/Handshake.hpp"

namespace
{
struct TestLogger
{
};
} // namespace

TEST(Handshake, request_headers_survive_moves)
{
    TestLogger logger;
    auto url = ws_client::URL::parse("wss://example.com");
    ASSERT_TRUE(url.has_value());

    ws_client::Handshake source(&logger, *url);
    source.get_request_header().fields.set("Authorization", "secret");

    ws_client::Handshake moved(std::move(source));
    auto authorization = moved.get_request_header().fields.get_first("Authorization");
    ASSERT_TRUE(authorization.has_value());
    EXPECT_EQ(*authorization, "secret");

    ws_client::Handshake assigned(&logger, *url);
    assigned = std::move(moved);
    authorization = assigned.get_request_header().fields.get_first("Authorization");
    ASSERT_TRUE(authorization.has_value());
    EXPECT_EQ(*authorization, "secret");
}

TEST(Handshake, validates_upgrade_headers)
{
    struct TestCase
    {
        std::string_view headers;
        bool valid;
    };

    constexpr std::array<TestCase, 8> cases{{
        {"Upgrade: websocket\r\nConnection: Upgrade\r\n", true},
        {"Upgrade: WebSocket\r\nConnection: keep-alive, Upgrade\r\n", true},
        {"Upgrade: h2c\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n", true},
        {"Upgrade: websocket\r\nConnection: keep-alive\r\nConnection: upgrade\r\n", true},
        {"Connection: Upgrade\r\n", false},
        {"Upgrade: h2c\r\nConnection: Upgrade\r\n", false},
        {"Upgrade: websocket\r\n", false},
        {"Upgrade: websocket\r\nConnection: keep-alive, Upgrader\r\n", false},
    }};

    ws_client::ConsoleLogger logger{ws_client::LogLevel::N};
    auto url = ws_client::URL::parse("wss://example.com");
    ASSERT_TRUE(url.has_value());

    for (const auto& test_case : cases)
    {
        ws_client::Handshake handshake(&logger, *url);
        (void)handshake.get_request_message();

        auto key = handshake.get_request_header().fields.get_first("Sec-WebSocket-Key");
        ASSERT_TRUE(key.has_value());

        ws_client::SHA1 checksum;
        checksum.update(*key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        auto sha1_bytes = checksum.final_bytes();
        std::string accept = ws_client::base64_encode(sha1_bytes.data(), sha1_bytes.size());

        std::string response = std::format(
            "HTTP/1.1 101 Switching Protocols\r\n{}Sec-WebSocket-Accept: {}\r\n\r\n",
            test_case.headers,
            accept
        );
        EXPECT_EQ(handshake.process_response(response).has_value(), test_case.valid)
            << test_case.headers;
    }
}

TEST(Handshake, formats_ipv6_host_header)
{
    ws_client::ConsoleLogger logger{ws_client::LogLevel::N};
    auto url = ws_client::URL::parse("wss://[::1]");
    ASSERT_TRUE(url.has_value());

    ws_client::Handshake handshake(&logger, *url);
    (void)handshake.get_request_message();

    EXPECT_EQ(handshake.get_request_header().fields.get_first("Host"), "[::1]:443");
}
