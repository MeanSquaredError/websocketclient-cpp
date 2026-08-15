#include "ws_client/ws_client.hpp"
#include "ws_client/ws_client_async.hpp"

int main()
{
    return ws_client::URL::parse("wss://example.com").has_value() ? 0 : 1;
}
