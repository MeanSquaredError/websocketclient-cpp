#pragma once

#include <expected>
#include <concepts>
#include <span>
#include <cstddef>
#include <chrono>
#include <optional>

#include "ws_client/errors.hpp"
#include "ws_client/Buffer.hpp"

namespace ws_client
{
using std::span;
using std::byte;

/**
 * Concept for asynchronous socket template type parameters based on coroutines.
 * Requires the socket to support reading and writing bytes to the underlying socket, and closing the socket.
 * The functions MUST NOT throw exceptions, and instead return WSError object.
 */
template <typename T, template <typename> typename TTask>
concept HasAsyncSocketOperations = requires(T t, span<byte> buffer) {
    /**
     * Reads data from socket into `buffer`.
     * Does not guarantee to fill buffer completely, partial reads are possible.
     * Returns the number of bytes read.
     */
    { t.read_some(buffer) } -> std::same_as<TTask<expected<size_t, WSError>>>;

    /**
     * Writes `buffer` to underlying socket.
     * Does not guarantee to write complete `buffer` to socket, partial writes are possible.
     * Returns the number of bytes written.
     */
    { t.write_some(buffer) } -> std::same_as<TTask<expected<size_t, WSError>>>;

    /**
     * Shuts down socket communication.
     * This function should be called before closing the socket
     * for a clean shutdown.
     * The return value in case of error may be ignored by the caller.
     * Safe to call multiple times.
     */
    { t.shutdown() } -> std::same_as<TTask<expected<void, WSError>>>;

    /**
     * Close the socket connection and all associated resources.
     * Safe to call multiple times.
     */
    { t.close() } -> std::same_as<TTask<expected<void, WSError>>>;
};

} // namespace ws_client