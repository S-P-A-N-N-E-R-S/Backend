#include <iostream>

#include <config/config.hpp>
#include <networking/io/io_server.hpp>
#include <scheduler/scheduler.hpp>

int main(int argc, const char **argv)
{
    // Parse configurations
    server::config_parser::instance().parse(argc, argv);

#ifdef SPANNERS_UNENCRYPTED_CONNECTION
    server::io_server server{
        server::config(server::config_options::SERVER_PORT).as<unsigned short>()};
#else
    if (server::config(server::config_options::TLS_CERT_PATH).empty() ||
        server::config(server::config_options::TLS_KEY_PATH).empty())
    {
        std::cerr << "[ERROR] No TLS certificate and key file given" << std::endl;
        return -1;
    }

    std::string cert_path{server::config(server::config_options::TLS_CERT_PATH).as<std::string>()};
    std::string key_path{server::config(server::config_options::TLS_KEY_PATH).as<std::string>()};
    server::io_server server{
        server::config(server::config_options::SERVER_PORT).as<unsigned short>(), cert_path,
        key_path};
#endif

    auto scheduler = &server::scheduler::instance();
    scheduler->start();

    server.run();
}
