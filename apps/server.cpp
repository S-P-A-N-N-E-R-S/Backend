#include <networking/io/io_server.hpp>
#include <scheduler/scheduler.hpp>

int main(int argc, const char **argv)
{
    auto scheduler = &server::scheduler::instance();
    scheduler->start();

    server::io_server server{4711};
    server.run();
}
