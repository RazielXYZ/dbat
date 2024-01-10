#include "portal/telnet.h"

namespace portal::telnet {
    // Telnet Options
    void TelnetOption::sendNegotiate(uint8_t code) {
        conn.sendBytes({codes::IAC, code, getCode()});
    }

    void TelnetOption::sendSubnegotiate(const std::vector<uint8_t>& data) {
        conn.sendSubnegotiate(getCode(), data);
    }

    void TelnetOption::start() {
        if(supportLocal()) {
            perspective.local.negotiating = true;
            sendNegotiate(codes::WILL);
        }
        if(supportRemote()) {
            perspective.remote.negotiating = true;
            sendNegotiate(codes::DO);
        }
    }

    void TelnetOption::onNegotiate(uint8_t negotiate) {
        using namespace codes;
        switch(negotiate) {
            case WILL:
                if(supportRemote()) {
                    auto &state = perspective.remote;
                    if(!state.enabled) {
                        state.enabled = true;
                        if(!state.negotiating) {
                            sendNegotiate(DO);
                        }
                        onRemoteEnable();
                    }
                } else {
                    sendNegotiate(DONT);
                }
            break;

            case DO:
                if(supportLocal()) {
                    auto &state = perspective.local;
                    if(!state.enabled) {
                        state.enabled = true;
                        if(!state.negotiating) {
                            sendNegotiate(WILL);
                        }
                        onLocalEnable();
                    }
                } else {
                    sendNegotiate(WONT);
                }
            break;

            case WONT:
                if(supportRemote()) {
                    auto &state = perspective.remote;
                    if(state.enabled) {
                        state.enabled = false;
                        onRemoteDisable();
                    }
                    if(state.negotiating) {
                        state.negotiating = false;
                        onRemoteReject();
                    }
                }
            break;

            case DONT:
                if(supportLocal()) {
                    auto &state = perspective.local;
                    if(state.enabled) {
                        state.enabled = false;
                        onLocalDisable();
                    }
                    if(state.negotiating) {
                        state.negotiating = false;
                        onLocalReject();
                    }
                }
            break;

            default:
                // Handle other cases or unknown command
                    break;
        }
    }

    // TelnetConnection

    TelnetConnection::TelnetConnection(StreamType stream, bool tls, boost::beast::flat_buffer buf,
        const any_io_executor &ex) : stream(std::move(stream)), tls(tls), readbuf(std::move(buf)), outMessage(ex),
        toGame(ex) {

        capabilities.tls = tls;
    }

    awaitable<void> TelnetConnection::run() {
        auto ex = co_await this_coro::executor;
        co_spawn(ex, runNegotiation(), boost::asio::detached);
        co_await (runReader() || runWriter());
        co_return;
    }

    awaitable<void> TelnetConnection::runWriter() {
        while(true) {
            auto message = co_await outMessage.async_receive(use_awaitable);

            switch(message.index()) {
                case 0: // raw application data: a vector of uint8_t
                    sendBytes(std::get<0>(message));
                    break;
                case 1: // a single byte, a telnet command. IAC <command>
                    sendBytes({codes::IAC, std::get<1>(message)});
                    break;
                case 2: // a negotiation pair, IAC <first> <second>
                {
                    auto pair = std::get<2>(message);
                    sendBytes({codes::IAC, std::get<0>(pair), std::get<1>(pair)});
                }
                    break;
                case 3: // iac subnegotiation.
                {
                    auto pair = std::get<3>(message);
                    // The first element of the pair is the option, the second is a vector of bytes. we need to send...
                    // IAC SB <option> <bytes> IAC SE
                    std::vector<uint8_t> bytes = {codes::IAC, codes::SB, std::get<0>(pair)};
                    bytes.insert(bytes.end(), std::get<1>(pair).begin(), std::get<1>(pair).end());
                    bytes.push_back(codes::IAC);
                    bytes.push_back(codes::SE);
                    sendBytes(bytes);
                }
                break;
            }

            // the sendBytes function put the bytes in writebuf. We need to flush it.
            if(stream.index()) {
                auto &tls = std::get<1>(stream);
                while(writebuf.size()) {
                    auto results = co_await tls.async_write_some(writebuf.data(), use_awaitable);
                    writebuf.consume(results);
                }
            } else {
                auto &tcp = std::get<0>(stream);
                while(writebuf.size()) {
                    auto results = co_await tcp.async_write_some(writebuf.data(), use_awaitable);
                    writebuf.consume(results);
                }
            }

        }

        co_return;
    }

    void TelnetConnection::sendBytes(const std::vector<uint8_t>& bytes) {
        // flush bytes to writebuf.
        writebuf.reserve(writebuf.size() + bytes.size());
        writebuf.commit(boost::asio::buffer_copy(writebuf.prepare(bytes.size()), boost::asio::buffer(bytes)));
    }


    awaitable<void> TelnetConnection::runReader() {
        while(true) {
            if(stream.index()) {
                auto &tls = std::get<1>(stream);
                auto results = co_await tls.async_read_some(readbuf.prepare(1024), use_awaitable);
                readbuf.commit(results);
            } else {
                auto &tcp = std::get<0>(stream);
                auto results = co_await tcp.async_read_some(readbuf.prepare(1024), use_awaitable);
                readbuf.commit(results);
            }

            while(co_await parseTelnet()) {

            }

        }

        co_return;
    }

    awaitable<bool> TelnetConnection::parseTelnet() {
        using namespace codes;
        auto data = static_cast<const uint8_t *>(readbuf.data().data());
        auto size = readbuf.size();

        if (size == 0) {
            co_return false;
        }

        if (data[0] != IAC) {
            // Handle raw app data
            size_t iac_pos = std::find(data, data + size, IAC) - data;
            std::vector<uint8_t> app_data(data, data + iac_pos);
            readbuf.consume(iac_pos);
            co_await handleApplicationData(app_data);
            co_return true;
        }

        // Handle Telnet commands
        if (size < 2) {
            // Need at least two bytes for any command
            co_return false;
        }

        uint8_t command = data[1];
        switch (command) {
            case IAC:
                readbuf.consume(2);
                co_await handleApplicationData({IAC});
                co_return true;

            case WILL:
            case WONT:
            case DO:
            case DONT:
                // Handle negotiation commands
                if (size < 3) {
                    // Need three bytes for negotiation
                    co_return false;
                }
                readbuf.consume(3);
                co_await handleNegotiation(command, data[2]);

                co_return true;

            case SB:
                // Handle subnegotiation
                if (size < 5) {
                    // Need at least five bytes for subnegotiation
                    co_return false;
                }
            // Find IAC SE sequence
                for (size_t i = 2; i < size - 1; ++i) {
                    if (data[i] == IAC && data[i + 1] == SE) {
                        // Found end of subnegotiation
                        std::vector<uint8_t> subneg_data(data + 2, data + i); // Exclude IAC SE
                        readbuf.consume(i + 2);
                        co_await handleSubnegotiation(data[1], subneg_data);
                        co_return true;
                    }
                }
            // IAC SE not found, need more data
                co_return false;

            default:
                readbuf.consume(2);
                co_await handleCommand(command);
                co_return true;
        }
    }

    awaitable<void> TelnetConnection::handleCommand(uint8_t command) {
        // this really does nothing at the moment.
        co_return;
    }

    awaitable<void> TelnetConnection::handleSubnegotiation(uint8_t option, const std::vector<uint8_t>& data) {
        if(const auto found = options.find(option); found != options.end()) {
            found->second->onSubnegotiation(data);
        }
        co_return;
    }

    void TelnetConnection::sendNegotiate(uint8_t negotiate, uint8_t option) {
        TelnetMessage msg(std::make_pair(negotiate, option));
        outMessage.try_send(boost::system::error_code{}, msg);
    }

    void TelnetConnection::sendSubnegotiate(uint8_t option, const std::vector<uint8_t> &data) {
        // we need to call sendbytes with a vector of bytes that is IAC SB <code> <data> IAC SE
        TelnetMessage msg(std::make_pair(option, data));
        outMessage.try_send(boost::system::error_code{}, msg);
    }

    void TelnetConnection::sendAppData(const std::vector<uint8_t>& data) {
        // This is basically just a byte send except we need to escape IAC bytes with an additional IAC.
        std::vector<uint8_t> bytes;
        bytes.reserve(data.size());
        for(auto byte : data) {
            if(byte == codes::IAC) {
                bytes.push_back(codes::IAC);
            }
            bytes.push_back(byte);
        }
        TelnetMessage msg(bytes);
        outMessage.try_send(boost::system::error_code{}, msg);
    }

    void TelnetConnection::sendCommand(uint8_t command) {
        TelnetMessage msg({command});
        outMessage.try_send(boost::system::error_code{}, msg);
    }

    awaitable<void> TelnetConnection::handleNegotiation(uint8_t negotiate, uint8_t option) {
        using namespace codes;
        if(const auto found = options.find(option); found != options.end()) {
            found->second->onNegotiate(negotiate);
            co_return;
        }

        switch(negotiate) {
            case WILL:
                sendNegotiate(DONT, option);
                break;
            case DO:
                sendNegotiate(WONT, option);
                break;
        }
        co_return;

    }

    awaitable<void> TelnetConnection::handleApplicationData(const std::vector<uint8_t>& data) {
        // first, we copy data into appbuf.
        appbuf.reserve(appbuf.size() + data.size());
        appbuf.commit(boost::asio::buffer_copy(appbuf.prepare(data.size()), boost::asio::buffer(data)));

        auto data = static_cast<const char*>(appbuf.data().data());
        auto size = appbuf.size();

        if (size == 0) {
            co_return;
        }

        size_t start = 0;
        while (start < size) {
            // Find the next occurrence of \r\n in the buffer
            size_t end = start;
            bool found = false;
            while (end < size - 1) {
                if (data[end] == '\r' && data[end + 1] == '\n') {
                    found = true;
                    break;
                }
                ++end;
            }

            if (!found) {
                // No complete line found, exit the loop
                break;
            }

            // Create a string from the line (excluding \r\n)
            std::string line(data + start, data + end);

            // Send the line for further processing
            co_await handleLine(std::move(line));

            // Move start position past the \r\n
            start = end + 2;
        }

        // Consume processed part of the buffer
        appbuf.consume(start);

        co_return;
    }


    void TelnetConnection::changeCapabilities(const nlohmann::json& j) {
        capabilities.deserialize(j);
    }

    awaitable<void> TelnetConnection::processAppData() {

    }





}