#include "portal/config.h"

namespace portal::config {
    std::string listenAddress{"0.0.0.0"};
    uint16_t listenPort{1280};

    std::string serverAddress{"127.0.0.1"};
    uint16_t serverPort{8000};

    std::string certPath, keyPath;
}