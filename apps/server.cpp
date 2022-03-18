#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

#include <config/config.hpp>
#include <networking/io/client_server.hpp>
#include <networking/io/management_server.hpp>
#include <scheduler/scheduler.hpp>

static void block_signals(sigset_t &sigset)
{
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
}

static std::string get_runtime_dir_path()
{
    static const char *runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    return (runtime_dir) ? runtime_dir : "/tmp";
}

int main(int argc, const char **argv)
{
    // Block signals to ensure correct shutdown of server processes
    sigset_t signal_set;
    block_signals(signal_set);

    server::config_parser::instance().parse(argc, argv);

    server::management_server m_server{get_runtime_dir_path() + "/spanners_server"};

#ifdef SPANNERS_UNENCRYPTED_CONNECTION
    server::client_server c_server{
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
    server::client_server c_server{
        server::config(server::config_options::SERVER_PORT).as<unsigned short>(), cert_path,
        key_path};
#endif

    server::scheduler::instance().start();

    c_server.start();
    m_server.start();

    // Wait for signal to terminate process
    std::cout << "[INFO] Press Ctrl-C to shutdown" << std::endl;
    int signal_num = 0;
    sigwait(&signal_set, &signal_num);

    // Shutdown servers on signal
    c_server.stop();
    m_server.stop();
    return 0;
}
