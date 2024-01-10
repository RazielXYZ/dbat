//
// Created by volund on 1/9/24.
//
#include "portal/config.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace portal::config;


int main(int argc, char* argv[]) {
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("listen", po::value<std::string>(), "address and port to listen on (format: addr:port)")
            ("server", po::value<std::string>(), "address and port of the server (format: addr:port)")
            ("cert", po::value<std::string>(), "path to the TLS certificate file")
            ("key", po::value<std::string>(), "path to the TLS key file");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        if (vm.count("listen")) {
            std::string addr = vm["listen"].as<std::string>();
            auto pos = addr.find(':');
            if (pos != std::string::npos) {
                listenAddress = addr.substr(0, pos);
                listenPort = static_cast<uint16_t>(std::stoi(addr.substr(pos + 1)));
            }
        }

        if (vm.count("server")) {
            std::string addr = vm["server"].as<std::string>();
            auto pos = addr.find(':');
            if (pos != std::string::npos) {
                serverAddress = addr.substr(0, pos);
                serverPort = static_cast<uint16_t>(std::stoi(addr.substr(pos + 1)));
            }
        }

        if (vm.count("cert")) {
            certPath = vm["cert"].as<std::string>();
        }

        if (vm.count("key")) {
            keyPath = vm["key"].as<std::string>();
        }

        // Display the configuration for verification
        std::cout << "Listening on: " << listenAddress << ":" << listenPort << std::endl;
        std::cout << "Server: " << serverAddress << ":" << serverPort << std::endl;
        if (!certPath.empty()) {
            std::cout << "TLS certificate: " << certPath << std::endl;
        }
        if (!keyPath.empty()) {
            std::cout << "TLS key: " << keyPath << std::endl;
        }

        // ... Continue with the rest of your program ...

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}