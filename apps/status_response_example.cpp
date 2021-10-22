#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/Dijkstra.h>

#include <handling/handler_factory.hpp>
#include <handling/handler_utilities.hpp>

#include <networking/messages/graph_message.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/requests/shortest_path_request.hpp>
#include <networking/responses/available_handlers_response.hpp>
#include <networking/responses/response_factory.hpp>
#include <networking/responses/shortest_path_response.hpp>

#include <meta.pb.h>
#include "status.pb.h"

using namespace boost::asio;
using ip::tcp;

int main(int argc, const char **argv)
{
    io_service io_service;
    tcp::socket socket(io_service);
    socket.connect(tcp::endpoint(ip::address::from_string("127.0.0.1"), 4711));

    // serialize & gzip it
    namespace io = boost::iostreams;
    graphs::MetaData meta;
    meta.set_type(graphs::RequestType::STATUS);
    meta.set_containersize(0);  // STATUS needs no RequestContainer

    uint64_t len = boost::endian::native_to_big(meta.ByteSizeLong());

    std::vector<char> meta_data;
    meta_data.resize(meta.GetCachedSize());
    meta.SerializeToArray(meta_data.data(), meta_data.size());

    // send length
    boost::system::error_code error;
    write(socket, buffer(reinterpret_cast<const char *>(&len), 8), error);
    if (error)
    {
        std::cout << "sending length failed: " << error.message() << std::endl;
        return 1;
    }
    // send meta data
    write(socket, buffer(meta_data.data(), meta_data.size()), error);
    if (error)
    {
        std::cout << "sending meta data failed: " << error.message() << std::endl;
    }

    // receive length
    size_t recv_size;
    boost::asio::read(socket, buffer(reinterpret_cast<char *const>(&recv_size), 8),
                      transfer_exactly(8), error);
    if (error)
    {
        std::cout << "reading length failed: " << error.message() << std::endl;
        return 1;
    }

    // receive data
    std::vector<char> recv_buffer;
    boost::endian::big_to_native_inplace(recv_size);
    recv_buffer.resize(recv_size);
    boost::asio::read(socket, buffer(recv_buffer.data(), recv_buffer.size()),
                      transfer_exactly(recv_buffer.size()), error);
    if (error)
    {
        std::cout << "reading meta data failed: " << error.message() << std::endl;
        return 1;
    }
    // parsing data
    graphs::MetaData response_meta_data;
    if (!response_meta_data.ParseFromArray(recv_buffer.data(), recv_buffer.size()))
    {
        std::cout << "parsing meta data failed" << std::endl;
        return 1;
    }

    graphs::ResponseContainer response_container;
    recv_buffer.clear();
    recv_buffer.resize(response_meta_data.containersize());
    boost::asio::read(socket, buffer(recv_buffer.data(), recv_buffer.size()),
                      transfer_exactly(recv_buffer.size()), error);
    if (error)
    {
        std::cout << "reading response data failed: " << error.message() << std::endl;
        return 1;
    }

    // dezip response
    io::filtering_streambuf<io::input> in_str_buf;
    in_str_buf.push(io::gzip_decompressor{});
    in_str_buf.push(io::array_source{recv_buffer.data(), recv_buffer.size()});

    // Copy compressed data into the decompressed buffer
    std::vector<char> decompressed;
    decompressed.reserve(recv_buffer.size());
    std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {}, std::back_inserter(decompressed));

    if (!response_container.ParseFromArray(decompressed.data(), decompressed.size()))
    {
        std::cout << "parsing response failed" << std::endl;
        return 1;
    }

    graphs::StatusResponse status_response;
    if (!response_container.response().UnpackTo(&status_response))
    {
        std::cout << "parsing status failed" << std::endl;
        return 1;
    }

    // output result
    for (auto &status : status_response.states())
    {
        //std::cout << available_handlers.handlers(i).name() << std::endl;
        std::cout << status.job_id() << ": " << status.status() << std::endl;
    }
}
