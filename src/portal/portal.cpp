//
// Created by volund on 1/9/24.
//
#include "portal/portal.h"
#include "portal/telnet.h"
#include "portal/webserver.h"

#include <thread>
#include <boost/asio/detached.hpp>
#include <boost/asio/strand.hpp>

#include <boost/asio/ssl.hpp>

namespace portal {

    boost::asio::io_context ioc;

    ssl::context sslContext{ssl::context::tlsv13_server};

    bool tlsEnabled;

    static bool is_http_request(const beast::flat_buffer& buf, std::size_t bytes_read) {
        std::string request_line(reinterpret_cast<const char*>(buf.data().data()), bytes_read);
        // List of common HTTP methods
        std::vector<std::string> methods = {"GET ", "POST ", "HEAD ", "PUT ", "DELETE ", "CONNECT ", "OPTIONS ", "TRACE ", "PATCH "};

        // Check if the start of the request line matches any HTTP method
        for (const auto& method : methods) {
            if (request_line.starts_with(method)) {
                return true;
            }
        }
        return false;
    }

    static awaitable<void> handleStream(StreamType stream, ip::basic_endpoint<tcp> ep, bool tls, beast::flat_buffer buf) {
        const auto timeout = std::chrono::milliseconds(200);
        boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor, timeout);
        bool timeout_expired = false;

        timer.async_wait([&](const boost::system::error_code& ec) {
            if (!ec) {
                timeout_expired = true;
                if(stream.index())
                    std::get<1>(stream).next_layer().cancel();
                else
                    std::get<0>(stream).cancel();
            }
        });

        bool telnet = false;

        std::size_t bytes_read = 0;
        try {
            if(stream.index())
                bytes_read = co_await std::get<1>(stream).async_read_some(buf.prepare(1024), use_awaitable);
            else
                bytes_read = co_await std::get<0>(stream).async_read_some(buf.prepare(1024), use_awaitable);
            buf.commit(bytes_read);
        } catch (...) {
            if (timeout_expired) {
                telnet = true;
                // Timeout occurred, assume Telnet
                // Handle as Telnet...
            } else {
                // Some other error occurred
                // Handle the error...
                co_return;
            }

        }

        if(bytes_read > 0 && is_http_request(buf, bytes_read)) {
            telnet = false;
        }

        auto ex = co_await this_coro::executor;

        if(telnet) {
            telnet::TelnetConnection telnetConnection(std::move(stream), tls, std::move(buf), ex);
            co_await telnetConnection.run();
        } else {
            webserver::WebserverConnection webserverConnection(std::move(stream), tls, std::move(buf));
            co_await webserverConnection.run();
        }

        co_return;
    }


    static awaitable<void> handleConnection(tcp::socket socket) {
        auto ep = socket.remote_endpoint();

        beast::flat_buffer buffer;

        if(tlsEnabled && co_await async_detect_ssl(socket, buffer, use_awaitable)) {
            beast::ssl_stream<tcp::socket> stream(std::move(socket), sslContext);
            co_await stream.async_handshake(ssl::stream_base::server, buffer.cdata(), use_awaitable);
            // we have a TLS connection!
            co_await handleStream(std::move(stream), ep, true, std::move(buffer));

        } else {
            // it is NOT a TLS connection but that's still okay!
            co_await handleStream(std::move(socket), ep, false, std::move(buffer));
        }

        co_return;
    }

    static awaitable<void> listen(tcp::acceptor& acceptor) {
        while(true) {
            auto socket = co_await acceptor.async_accept(use_awaitable);
            co_spawn(make_strand(ioc), handleConnection(std::move(socket)), detached);
        }

        co_return;
    }

    void run() {
        if(!config::certPath.empty()) {
            sslContext.use_certificate_chain_file(config::certPath);
            if(!config::keyPath.empty()) {
                sslContext.use_private_key_file(config::keyPath, ssl::context::pem);
                tlsEnabled = true;
            }
        }

        tcp::acceptor acceptor(ioc, {tcp::v4(), config::listenPort});
        co_spawn(ioc, listen(acceptor), boost::asio::detached);

        // Use as many threads as there are CPUs
        const auto thread_count = std::min<int>(0, std::thread::hardware_concurrency() - 1);
        std::vector<std::thread> threads;

        if(thread_count) {
            threads.reserve(thread_count);
            for (auto i = 0u; i < thread_count; ++i) {
                threads.emplace_back([&] { ioc.run(); });
            }
        }

        ioc.run();

        // Wait for all threads to complete
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        threads.clear();
    }

}
