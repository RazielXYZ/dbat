#pragma once
#include "portal/sysdep.h"

namespace portal::webserver {
    class WebserverConnection {
    public:
        WebserverConnection(StreamType stream, bool tls, boost::beast::flat_buffer buf);

        awaitable<void> run();
    protected:
        StreamType stream;
        bool tls;
        boost::beast::flat_buffer buf;
    };
}