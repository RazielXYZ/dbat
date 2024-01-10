#include "dbat/net.h"
#include "dbat/utils.h"
#include <regex>
#include "dbat/screen.h"
#include "dbat/players.h"
#include "dbat/login.h"
#include "dbat/config.h"
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

namespace net {
    using namespace boost::asio::experimental::awaitable_operators;
    std::unique_ptr<io_context> io;
    std::unique_ptr<signal_set> signals;

    std::mutex connectionsMutex, pendingConnectionsMutex;
    std::unique_ptr<Link> link;
    std::unique_ptr<Channel<nlohmann::json>> linkChannel;
    boost::asio::ip::tcp::endpoint thermiteEndpoint;

    std::map<int64_t, std::shared_ptr<Connection>> connections;
    std::set<int64_t> pendingConnections;

    std::unordered_map<int64_t, DisconnectReason> deadConnections;

    Link::Link(boost::beast::websocket::stream<boost::beast::tcp_stream> ws)
            : conn(std::move(ws)), is_stopped(false) {}

    awaitable<void> Link::run() {
        try {
            co_await (runReader() || runWriter() || runPinger());
        } catch (const boost::system::system_error& e) {
            logger->error("Error in Link::run(): {}", e.what());
            // Handle exceptions here if necessary
        } catch(...) {
            logger->error("Unknown error in Link::run()");
        }
        if(conn.is_open()) {
            try {
                conn.close(boost::beast::websocket::close_code::normal);
            } catch(...) {
                logger->error("Error closing WebSocket gracefully...");
            }
        }
        co_return;
    }

    awaitable<void> Link::runPinger() {
        boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
        while(!is_stopped) {
            timer.expires_after(100ms);
            co_await timer.async_wait(boost::asio::use_awaitable);
            if(!is_stopped) {
                // use async_ping to send a ping...
                co_await conn.async_ping(boost::beast::websocket::ping_data{}, boost::asio::use_awaitable);
            }
        }
        co_return;
    }

    void Link::stop() {
        is_stopped = true;
    }

    awaitable<void> Link::createUpdateClient(const nlohmann::json &j) {
        auto id = j["id"].get<int64_t>();
        const auto& capabilities = j["capabilities"];

        // Do something with id, addr, and capabilities
        auto it = connections.find(id);
        if (it == connections.end()) {
            // Create a new ClientConnection
            auto cc = std::make_shared<Connection>(id);
            cc->capabilities.deserialize(capabilities);
            if(config::usingMultithreading) {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                connections[id] = cc;
            } else connections[id] = cc;

            if(config::usingMultithreading) {
                std::lock_guard<std::mutex> lock(pendingConnectionsMutex);
                pendingConnections.insert(id);
            } else pendingConnections.insert(id);
        } else {
            // Update the existing ClientConnection
            auto& client_connection = it->second;
            client_connection->capabilities.deserialize(capabilities);
        }
        co_return;
    }

    awaitable<void> Link::runReader() {
        while (!is_stopped) {
            try {
                // Read a message from the WebSocket
                boost::beast::flat_buffer buffer;
                co_await conn.async_read(buffer, boost::asio::use_awaitable);

                // Deserialize the JSON string
                auto ws_str = boost::beast::buffers_to_string(buffer.data());
                //std::cout << "Received: " << ws_str << std::endl;
                auto j = nlohmann::json::parse(ws_str);


                // Access the "kind" field in the JSON object
                std::string kind = j["kind"];

                // Implement your routing logic here

                if (kind == "client_list") {
                    // This message is sent by Thermite when the game establishes a fresh connection with it.
                    // It should be the first thing a Link sees.
                    //logger->info("Link: Received client_list message");
                    // Get the "data" object
                    auto &data = j["data"];

                    // Iterate over the contents of the "data" object
                    for (const auto &entry : data) {
                        co_await createUpdateClient(entry);
                    }

                } else if (kind == "client_ready") {
                    // This message is sent by Thermite when a new client has connected.
                    auto &data = j["protocol"];
                    co_await createUpdateClient(data);

                } else {
                    // Extract the "id" field from the JSON object
                    int64_t id = j["id"];

                    // Look up the specific ClientConnection in the std::map
                    auto it = connections.find(id);
                    if(it == connections.end()) {
                        logger->info("Link: Received message for unknown client: {}", id);
                        continue;
                    }

                    // Found the client connection
                    auto &client_connection = it->second;

                    if (kind == "client_capabilities") {
                        auto &capabilities = j["capabilities"];
                        client_connection->capabilities.deserialize(capabilities);

                    } else if (kind == "client_data") {
                        try {
                            co_await client_connection->fromLink.async_send(boost::system::error_code{}, j, boost::asio::use_awaitable);
                        } catch (const boost::system::system_error &e) {
                            // Handle exceptions (e.g., WebSocket close or error)
                        }

                    } else if (kind == "client_disconnected") {
                        logger->info("Link: Received client_disconnected message for client: {}", id);
                        deadConnections[id] = DisconnectReason::ConnectionClosed;
                    }
                }
            } catch (const boost::system::system_error &e) {
                logger->error("Link RunReader flopped at: {}", e.what());
                break;
                // Handle exceptions (e.g., WebSocket close or error)
            } catch (...) {
                logger->error("Unknown error in Link RunReader");
                break;
            }
        }
        co_return;
    }

    awaitable<void> Link::runWriter() {
        while (!is_stopped) {
            try {
                // Receive a message from the channel asynchronously
                auto message = co_await linkChannel->async_receive(boost::asio::use_awaitable);

                try {
                    // Serialize the JSON message to text
                    std::string serialized_msg = message.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);

                    // Send the message across the WebSocket
                    co_await conn.async_write(boost::asio::buffer(serialized_msg), boost::asio::use_awaitable);
                } catch (const boost::system::system_error& e) {
                    logger->error("Link runWriter flopped 1: {}", e.what());
                    // Handle exceptions (e.g., WebSocket close or error) when sending the message
                } catch (...) {
                    logger->error("Unknown error in Link runWriter 1");
                }
            } catch (const boost::system::system_error& e) {
                logger->error("Link runWriter flopped 2: {}", e.what());
                // Handle exceptions (e.g., error receiving the message from the channel)
                break;
            } catch (...) {
                logger->error("Unknown error in Link runWriter 2");
                break;
            }
        }
        co_return;
    }



    awaitable<void> runLinkManager() {
        bool do_standoff = false;
        boost::asio::steady_timer standoff(co_await boost::asio::this_coro::executor);
        while (true) {
            if(do_standoff) {
                standoff.expires_after(5s);
                co_await standoff.async_wait(boost::asio::use_awaitable);
                do_standoff = false;
            }

            auto &endpoint = thermiteEndpoint;

            try {
                logger->info("LinkManager: Connecting to {}:{}...", endpoint.address().to_string(), endpoint.port());
                // Create a new TCP socket
                boost::beast::websocket::stream<boost::beast::tcp_stream> ws(co_await boost::asio::this_coro::executor);

                // Connect to the endpoint
                co_await boost::beast::get_lowest_layer(ws).async_connect(endpoint, boost::asio::use_awaitable);
                // Initialize a WebSocket using the connected socket

                // Perform the WebSocket handshake
                co_await ws.async_handshake(endpoint.address().to_string() + ":" + std::to_string(endpoint.port()), "/", boost::asio::use_awaitable);

                // Construct a Link using the WebSocket
                link = std::make_unique<Link>(std::move(ws));

                // Run the Link
                logger->info("LinkManager: Link established! Running Link...");
                co_await link->run();
                link.reset();

            } catch (const boost::system::system_error& error) {
                // If there was an error, handle it (e.g., log the error message)
                logger->error("Error in LinkManager::run(): {}", error.what());

                // You might want to add a delay before attempting to reconnect, e.g.,
                do_standoff = true;
            }
        }
        co_return;
    }

    void Connection::sendMessage(const Message &msg) {
        nlohmann::json j, d;
        j["kind"] = "client_data";
        j["id"] = this->connId;

        d["cmd"] = msg.cmd;
        d["args"] = msg.args;
        d["kwargs"] = msg.kwargs;

        j["data"].push_back(d);

        linkChannel->try_send(boost::system::error_code{}, j);
    }

    void Connection::sendText(const std::string &text) {
        if(text.empty()) return;
        Message msg;
        msg.cmd = "text";
        if(boost::icontains(text, "\r\r\n")) {
            logger->info("WHAT THE HELL?");
        }
        if(desc && desc->character) {
            auto &p = players[desc->character->id];
            msg.args.push_back(processColors(text, COLOR_ON(desc->character), p.color_choices));
        } else {
            msg.args.push_back(processColors(text, true, nullptr));
        }
        sendMessage(msg);
    }

    void Connection::handleMessage(const Message &m) {
        if(m.cmd == "text") {
            for(auto &arg : m.args) {
                if(arg.is_string()) {
                    auto t = arg.get<std::string>();
                    if(boost::iequals(t, "idle")) continue;
                    if(parser) parser->parse(t);
                }
            }
        } else {
            if(parser) parser->handleMessage(m);
        }
    }

    void Connection::onWelcome() {
        account = nullptr;
        desc = nullptr;
        setParser(new LoginParser(shared_from_this()));
    }

    void Connection::close() {
        deadConnections[this->connId] = DisconnectReason::GameLogoff;
    }

    void Connection::onNetworkDisconnected() {
        deadConnections[this->connId] = DisconnectReason::ConnectionLost;
    }


    Connection::Connection(int64_t connId) : connId(connId), fromLink(*io, 200) {

    }

    void Connection::cleanup(DisconnectReason reason) {
        if(account) {
            // Remove ourselves from the account's connections list.
            account->connections.erase(this);
            account = nullptr;
        }
        switch(reason) {
            case DisconnectReason::ConnectionLost:
                // The connection to remote host was lost for unknown reasons.
                // The portal is aware of this and already cleaned them up.
                if(desc) {
                    // If we have a desc then we inform it of the unexpected disconnect.
                    // It can handle this as it sees fit.
                    // onConnectionLost must remove the connection from the descriptor.
                    desc->onConnectionLost(connId);
                    desc = nullptr;
                }
                break;
            case DisconnectReason::ConnectionClosed:
                // The remote socket gracefully/voluntarily closed by the remote host.
                // The portal is aware of this and already cleaned them up.
                if(desc) {
                    // If we have a desc then we can attempt a 'quit' on the
                    // connected character. OR however else it wants to handle it.
                    // onConnectionClosed must remove the connection from the descriptor.
                    desc->onConnectionClosed(connId);
                    desc = nullptr;
                }
                break;
            case DisconnectReason::GameLogoff: {
                // The game server is initiating disconnection for some reason or another.
                // The portal must be informed so it can boot the remote host.
                nlohmann::json j;
                j["kind"] = "client_disconnected";
                j["id"] = connId;
                j["reason"] = "logoff";
                linkChannel->try_send(boost::system::error_code{}, j);
                }
                break;
        }

        // We do not remove ourselves from net::connections; that's done by the main loop.

    }

    void Connection::onHeartbeat(double deltaTime) {
        // Every time the heartbeat runs, we want to pull everything out of fromLink
        // first of all.

        // We need to do this in a loop, because we may have multiple messages in the
        // channel.

        std::error_code ec;
        nlohmann::json value;
        while (fromLink.ready()) {
            if(!fromLink.try_receive([&](std::error_code ec2, nlohmann::json value2) {
                ec = ec2;
                value = value2;
            })) {
                break;
            }
            if (ec) {
                // TODO: Handle any errors..
            } else {
                // If we got a json value, it should look like this.
                // {"kind": "client_data", "id": 123, "data": [{"cmd": "text", "args": ["hello world!"], "kwargs": {}}]}
                // We're only interested in iterating over the contents of the "data" array.
                // Now we must for-each over the contents of jarr, extract the cmd, args, and kwargs data, and call
                // the appropriate handle routine.
                //logger->info("Received data from link: {}", value.dump());
                lastActivity = std::chrono::steady_clock::now();
                for (auto &jval : value["data"]) {
                    //logger->info("Processing data from link: {}", jval.dump());
                    Message m(jval);
                    handleMessage(m);
                }
            }
        }
    }


    void ConnectionParser::handleMessage(const Message &m) {
        // The default does nothing, but later it will handle API stuff.
    }

    void ConnectionParser::close() {

    }

	void ConnectionParser::sendText(const std::string &txt) {
        conn->sendText(txt);
    }

    void ConnectionParser::start() {

    }

    void Connection::setParser(ConnectionParser *p) {
        if(parser) parser->close();
        parser.reset(p);
        p->start();
    }

}