#include <iostream>

#include <networking/io/io_server.hpp>
#include <scheduler/scheduler.hpp>

int main(int argc, const char **argv)
{
#ifdef SPANNERS_UNENCRYPTED_CONNECTION
    server::io_server server{4711};
#else
    // TODO Use config file when implemented to get the cert and key path
    // Currently first argument represents the cert path and the second one represents the key path

    if (argc < 3)
    {
        std::cerr << "[ERROR] No TLS certificat and key file given" << std::endl;
        return -1;
    }

    std::string cert_path{argv[1]};
    std::string key_path{argv[2]};
    server::io_server server{4711, cert_path, key_path};
#endif

    auto scheduler = &server::scheduler::instance();
    scheduler->start();

    server.run();
}
