#include <iostream>
#include <signal.h>
#include <string>
#include <variant>
#include <chrono>

#include <asio.hpp>
#include <asio/read_until.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/ssl.hpp>

#define WS_CLIENT_LOG_HANDSHAKE 1
#define WS_CLIENT_LOG_MSG_PAYLOADS 1
#define WS_CLIENT_LOG_MSG_SIZES 1
#define WS_CLIENT_LOG_FRAMES 1
#define WS_CLIENT_LOG_COMPRESSION 0

#include "ws_client/ws_client_async.hpp"
#include "ws_client/transport/AsioSocket.hpp"
#include "ws_client/PermessageDeflate.hpp"

using namespace ws_client;

asio::awaitable<expected<void, WSError>> run()
{
    // parse URL
    WS_CO_TRY(url_res, URL::parse("wss://localhost:9443"));
    URL& url = *url_res;

    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::resolver resolver(executor);
    auto endpoints = co_await resolver.async_resolve(url.host(), "9443", asio::use_awaitable);


    asio::ssl::context ctx(asio::ssl::context::tlsv12_client);
    ctx.load_verify_file("cert.pem");
    ctx.set_verify_mode(asio::ssl::verify_peer);
    ctx.set_verify_callback(asio::ssl::host_name_verification(url.host()));

    std::cout << "Connecting to " << url.host() << "... \n";
    asio::ssl::stream<asio::ip::tcp::socket> socket(executor, ctx);
    co_await asio::async_connect(socket.lowest_layer(), endpoints, asio::use_awaitable);
    std::cout << "Connected\n";

    co_await socket.async_handshake(asio::ssl::stream_base::client, asio::use_awaitable);
    std::cout << "Handshake ok\n";

    // websocketclient logger
    ConsoleLogger<LogLevel::D> logger;

    auto asio_socket = AsioSocket(&logger, std::move(socket));

    // websocket client
    auto client = WebSocketClientAsync<asio::awaitable, decltype(logger), decltype(asio_socket)>(
        &logger, std::move(asio_socket)
    );

    // handshake handler
    auto handshake = Handshake(&logger, url);

    // start client
    WS_CO_TRYV(co_await client.init(handshake));

    // send message
    string payload = "test";
    Message msg(MessageType::TEXT, payload);
    WS_CO_TRYV(co_await client.send_message(msg));

    Buffer buffer;
    for (int i = 0;; i++)
    {
        // read message from server into buffer
        variant<Message, PingFrame, PongFrame, CloseFrame, WSError> var = //
            co_await client.read_message(buffer);

        if (std::get_if<Message>(&var))
        {
            // write message back to server
            string text = "This is the " + std::to_string(i) + "th message";
            Message msg2(MessageType::TEXT, text);
            WS_CO_TRYV(co_await client.send_message(msg2));
        }
        else if (auto ping_frame = std::get_if<PingFrame>(&var))
        {
            logger.log<LogLevel::D>("Ping frame received");
            WS_CO_TRYV(co_await client.send_pong_frame(ping_frame->payload_bytes()));
        }
        else if (std::get_if<PongFrame>(&var))
        {
            logger.log<LogLevel::D>("Pong frame received");
        }
        else if (auto close_frame = std::get_if<CloseFrame>(&var))
        {
            // server initiated close
            if (close_frame->has_reason())
            {
                logger.log<LogLevel::I>(
                    "Close frame received: " + string(close_frame->get_reason())
                );
            }
            else
                logger.log<LogLevel::I>("Close frame received");
            break;
        }
        else if (auto err = std::get_if<WSError>(&var))
        {
            // error occurred - must close connection
            logger.log<LogLevel::E>("Error: " + err->message);
            WS_CO_TRYV(co_await client.close(err->close_with_code));
            co_return expected<void, WSError>{};
        }
    }

    WS_CO_TRYV(co_await client.close(close_code::NORMAL_CLOSURE));

    co_return expected<void, WSError>{};
};


int main()
{
    asio::io_context ctx;

    auto exception_handler = [&](auto e_ptr)
    {
        if (e_ptr)
            std::rethrow_exception(e_ptr);
    };

    auto client = []() -> asio::awaitable<void>
    {
        try
        {
            auto res = co_await run();
            if (!res.has_value())
                std::cerr << "Error: " << res.error().message << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    };

    asio::co_spawn(ctx, client, std::move(exception_handler));
    ctx.run();

    return 0;
}
