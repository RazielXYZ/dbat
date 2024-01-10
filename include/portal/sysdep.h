#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <coroutine>
#include <optional>
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/detached.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include "shared/net.h"

namespace portal {
    namespace beast = boost::beast;         // from <boost/beast.hpp>
    namespace http = beast::http;           // from <boost/beast/http.hpp>
    namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
    using namespace boost::asio;
    using tcp = ip::tcp;       // from <boost/asio/ip/tcp.hpp>
    using namespace boost::asio::experimental::awaitable_operators;
    using StreamType = std::variant<ip::tcp::socket, beast::ssl_stream<ip::tcp::socket>>;

    template<typename T>
    using Channel = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, T)>;

    extern boost::asio::io_context ioc;


}