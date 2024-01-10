#pragma once
#include "portal/sysdep.h"

namespace portal::telnet {
    namespace codes {
        constexpr uint8_t NUL = 0, BEL = 7, CR = 13, LF = 10, SGA = 3, TELOPT_EOR = 25, NAWS = 31;
        constexpr uint8_t LINEMODE = 34, EOR = 239, SE = 240, NOP = 241, GA = 249, SB = 250;
        constexpr uint8_t WILL = 251, WONT = 252, DO = 253, DONT = 254, IAC = 255, MNES = 39;
        constexpr uint8_t MXP = 91, MSSP = 70, MCCP2 = 86, MCCP3 = 87, GMCP = 201, MSDP = 69;
        constexpr uint8_t MTTS = 24;
    }


    using TelnetMessage = std::variant<std::vector<uint8_t>, uint8_t, std::pair<uint8_t, uint8_t>, std::pair<uint8_t, std::vector<uint8_t>>>;

    class TelnetConnection;

    struct TelnetOptionState {
        bool negotiating{false};
        bool enabled{false};
    };

    struct TelnetOptionPerspective {
        TelnetOptionState local, remote;
    };

    class TelnetOption {
    public:
        TelnetOption(TelnetConnection &conn) : conn(conn) {

        }
        virtual uint8_t getCode() = 0;
        void start();
        void sendNegotiate(uint8_t code);
        void sendSubnegotiate(const std::vector<uint8_t>& data);
        virtual bool supportLocal() { return false; }
        virtual bool supportRemote() { return false; }
        virtual void onLocalEnable() {};
        virtual void onRemoteEnable() {};
        virtual void onRemoteDisable() {};
        virtual void onLocalDisable() {};
        virtual void onLocalReject() {};
        virtual void onRemoteReject() {};
        virtual void onSubnegotiation(const std::vector<uint8_t> &data) {};
        void onNegotiate(uint8_t negotiate);
    protected:
        TelnetConnection &conn;
        TelnetOptionPerspective perspective;

    };

    class TelnetConnection {
    public:
        TelnetConnection(StreamType stream, bool tls, boost::beast::flat_buffer buf, const any_io_executor& ex);

        awaitable<void> run();

        void sendBytes(const std::vector<uint8_t> &bytes);
        void sendNegotiate(uint8_t negotiate, uint8_t option);
        void sendSubnegotiate(uint8_t option, const std::vector<uint8_t> &data);
        void sendAppData(const std::vector<uint8_t> &data);
        void sendCommand(uint8_t command);

        void changeCapabilities(const nlohmann::json& j);

    protected:
        net::ProtocolCapabilities capabilities;
        awaitable<void> runReader();
        awaitable<void> runWriter();
        awaitable<void> runNegotiation();
        awaitable<bool> parseTelnet();

        awaitable<void> handleApplicationData(const std::vector<uint8_t> &data);
        awaitable<void> handleCommand(uint8_t command);
        awaitable<void> handleNegotiation(uint8_t negotiate, uint8_t option);
        awaitable<void> handleSubnegotiation(uint8_t option, const std::vector<uint8_t> &data);

        awaitable<void> processAppData();

        std::unordered_map<uint8_t, std::unique_ptr<TelnetOption>> options;
        StreamType stream;
        bool tls;
        boost::beast::flat_buffer readbuf, writebuf, appbuf;
        Channel<TelnetMessage> outMessage;
        Channel<net::GameMessage> toGame;

        bool started{false};
    };


}