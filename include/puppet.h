#include "net.h"

namespace net {
    class PuppetParser : public ConnectionParser {
    public:
        PuppetParser(struct connection_data *co, char_data *c);
        void parse(const std::string &txt) override;
        void start() override;
    protected:
        char_data *ch;
    };
}