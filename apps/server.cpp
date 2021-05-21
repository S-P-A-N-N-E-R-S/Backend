#include <networking/io/io_server.hpp>

int main(int argc, const char **argv)
{
    server::io_server server{4711};
    server.run();
}
