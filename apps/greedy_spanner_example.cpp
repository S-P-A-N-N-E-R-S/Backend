#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/graphalg/Dijkstra.h>

#include <handling/handler_factory.hpp>
#include <handling/handler_proxy.hpp>

#include <networking/messages/graph_message.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/requests/shortest_path_request.hpp>
#include <networking/responses/available_handlers_response.hpp>
#include <networking/responses/response_factory.hpp>
#include <networking/responses/shortest_path_response.hpp>

#include <generic_container.pb.h>
#include <meta.pb.h>
#include <new_job_response.pb.h>
#include <result.pb.h>
#include <status.pb.h>

using namespace boost::asio;
using ip::tcp;
namespace io = boost::iostreams;

int main(int argc, const char **argv)
{
    int job_id{};
    {
        io_service io_service;
        tcp::socket socket(io_service);
        socket.connect(tcp::endpoint(ip::address::from_string("127.0.0.1"), 4711));

        graphs::RequestContainer proto_request_container;
        {
            graphs::GenericRequest proto_request;

            auto og = std::make_unique<ogdf::Graph>();
            ogdf::randomSimpleConnectedGraph(*og, 100, 1000);

            auto node_uids = std::make_unique<ogdf::NodeArray<server::uid_t>>(*og);
            auto *node_coords = proto_request.mutable_vertexcoordinates();

            for (const auto &node : og->nodes)
            {
                const auto uid = node->index();

                (*node_uids)[node] = uid;
                node_coords->Add([] {
                    auto coords = graphs::VertexCoordinates{};
                    coords.set_x(ogdf::randomDouble(-50, 50));
                    coords.set_y(ogdf::randomDouble(-50, 50));
                    coords.set_z(ogdf::randomDouble(-50, 50));

                    return coords;
                }());
            }

            auto edge_uids = std::make_unique<ogdf::EdgeArray<server::uid_t>>(*og);
            auto *edge_costs = proto_request.mutable_edgecosts();

            for (const auto &edge : og->edges)
            {
                const auto uid = edge->index();

                (*edge_uids)[edge] = uid;
                edge_costs->Add(ogdf::randomDouble(0, 100));
            }

            auto proto_graph = std::make_unique<graphs::Graph>(
                server::graph_message(std::move(og), std::move(node_uids), std::move(edge_uids))
                    .as_proto());

            proto_request.set_allocated_graph(proto_graph.release());

            // Hardcoded for testing purposes only
            proto_request.mutable_graphattributes()->operator[]("stretch") = "1.5";

            proto_request_container.mutable_request()->PackFrom(proto_request);
        }

        std::vector<char> container_data;
        {
            // Generate uncompressed buffer from protobuf container
            std::vector<char> uncompressed;
            uncompressed.resize(proto_request_container.ByteSizeLong());
            proto_request_container.SerializeToArray(uncompressed.data(), uncompressed.size());

            // Boost gzip compression
            io::filtering_streambuf<io::input> in_str_buf;
            in_str_buf.push(io::gzip_compressor{});
            in_str_buf.push(io::array_source{uncompressed.data(), uncompressed.size()});

            // Copy compressed data into the compressed buffer
            std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                      std::back_inserter(container_data));
        }

        // serialize & gzip it
        graphs::MetaData meta;
        meta.set_type(graphs::RequestType::GENERIC);
        meta.set_containersize(container_data.size());
        meta.set_handlertype("greedy_spanner");
        meta.set_jobname("Greedy Spanner Example");

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
        // send real data
        write(socket, buffer(container_data.data(), container_data.size()), error);
        if (error)
        {
            std::cout << "sending real data failed: " << error.message() << std::endl;
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

        graphs::NewJobResponse njr;
        {
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
            std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                      std::back_inserter(decompressed));

            if (!response_container.ParseFromArray(decompressed.data(), decompressed.size()))
            {
                std::cout << "parsing response failed" << std::endl;
                return 1;
            }

            if (!response_container.response().UnpackTo(&njr))
            {
                std::cout << "unpacking NewJobResponse failed" << std::endl;
                return 1;
            }
        }

        job_id = njr.jobid();
    }

    {
        // Periodically ask for job status until it's finished
        auto status = graphs::StatusType::UNKNOWN_STATUS;
        while (true)
        {
            io_service io_service;
            tcp::socket socket(io_service);
            socket.connect(tcp::endpoint(ip::address::from_string("127.0.0.1"), 4711));

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
            std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                      std::back_inserter(decompressed));

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

            for (const auto &s : status_response.states())
            {
                if (s.job_id() == job_id)
                {
                    status = s.status();
                }
            }

            if (status == graphs::StatusType::SUCCESS || status == graphs::StatusType::FAILED ||
                status == graphs::StatusType::ABORTED)
            {
                break;
            }

            std::cout << "Status was " << status << ", sleeping ..." << std::endl;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(500ms);
        }

        std::cout << "Status was " << status << ", requesting result ..." << std::endl;
    }

    // Server told us the result is ready
    {
        io_service io_service;
        tcp::socket socket(io_service);
        socket.connect(tcp::endpoint(ip::address::from_string("127.0.0.1"), 4711));

        graphs::ResultRequest res_req;
        res_req.set_jobid(job_id);

        graphs::RequestContainer container;
        container.mutable_request()->PackFrom(res_req);

        std::vector<char> container_data;
        {
            // Generate uncompressed buffer from protobuf container
            std::vector<char> uncompressed;
            uncompressed.resize(container.ByteSizeLong());
            container.SerializeToArray(uncompressed.data(), uncompressed.size());

            // Boost gzip compression
            io::filtering_streambuf<io::input> in_str_buf;
            in_str_buf.push(io::gzip_compressor{});
            in_str_buf.push(io::array_source{uncompressed.data(), uncompressed.size()});

            // Copy compressed data into the compressed buffer
            std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                      std::back_inserter(container_data));
        }

        // serialize & gzip it
        graphs::MetaData meta;
        meta.set_type(graphs::RequestType::RESULT);
        meta.set_containersize(container_data.size());

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
        // send real data
        write(socket, buffer(container_data.data(), container_data.size()), error);
        if (error)
        {
            std::cout << "sending real data failed: " << error.message() << std::endl;
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
        std::cout << "Received finished job: " << response_meta_data.jobname() << '\n';

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
        std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                  std::back_inserter(decompressed));

        if (!response_container.ParseFromArray(decompressed.data(), decompressed.size()))
        {
            std::cout << "parsing response failed" << std::endl;
            return 1;
        }

        graphs::GenericResponse resp;
        response_container.response().UnpackTo(&resp);
        std::cout << "response handler type: " << response_meta_data.handlertype() << "\n";

        const double graph_weight =
            std::accumulate(resp.edgecosts().begin(), resp.edgecosts().end(), 0.0);

        std::cout << "parsed graph nof nodes: " << resp.graph().vertexlist_size() << "\n";
        std::cout << "parsed graph nof edges: " << resp.graph().edgelist_size() << "\n";
        std::cout << "weight of parsed graph: " << graph_weight << "\n";
    }
}
