#include "portal/webserver.h"

namespace portal::webserver {
    WebserverConnection::WebserverConnection(StreamType stream, bool tls, boost::beast::flat_buffer buf) :
    stream(std::move(stream)), tls(tls), buf(std::move(buf)) {

    }


}