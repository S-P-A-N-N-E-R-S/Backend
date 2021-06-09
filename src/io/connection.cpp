#include <networking/io/connection.hpp>

#include <algorithm>
#include <boost/asio/completion_condition.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>

#include <handling/handler_proxy.hpp>
#include <networking/io/connection_handler.hpp>
#include <networking/requests/request_factory.hpp>
#include <networking/responses/response_factory.hpp>

using boost::asio::async_read;
using boost::asio::buffer;
using boost::asio::transfer_exactly;
using boost::asio::ip::tcp;
using boost::system::error_code;

namespace server {

namespace {
    constexpr int LENGHT_FIELD_SIZE = 8;
}

connection::connection(size_t id, connection_handler &handler, tcp::socket sock)
    : m_identifier{id}
    , m_handler{handler}
    , m_sock{std::move(sock)}
{
}

connection::~connection()
{
    m_sock.close();
}

void connection::handle()
{
    read();
}

void connection::read()
{
    async_read(m_sock, buffer(&m_size, LENGHT_FIELD_SIZE), transfer_exactly(8),
               [this](error_code err, size_t len) {
                   if (err)
                   {
                       std::cout << "ERROR_READ_SIZE\n";
                       m_handler.remove(m_identifier);
                       return;
                   }

                   boost::endian::big_to_native_inplace(m_size);

                   try
                   {
                       namespace io = boost::iostreams;

                       // TODO Discuss to use a max size for messages
                       m_buffer.resize(m_size);
                       std::cout << "Recv size: " << m_size << '\n';

                       // Read next <size> bytes and create graphs::RequestContainer
                       async_read(
                           m_sock, buffer(m_buffer), transfer_exactly(m_size),
                           [this](error_code err, size_t len) {
                               if (err)
                               {
                                   std::cout << "ERROR_READ_PROTO\n";
                                   m_handler.remove(m_identifier);
                                    return;
                               }

                               // Boost gzip decompression
                               io::filtering_streambuf<io::input> in_str_buf;
                               in_str_buf.push(io::gzip_decompressor{});
                               in_str_buf.push(io::array_source{m_buffer.data(), m_buffer.size()});

                               // Copy compressed data into the decompressed buffer
                               std::vector<char> decompressed;
                               decompressed.reserve(m_buffer.size());
                               std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                                         std::back_inserter(decompressed));

                               graphs::RequestContainer request_container;
                               if (request_container.ParseFromArray(decompressed.data(),
                                                                    decompressed.size()))
                               {
                                   std::cout << "Parsed proto message!\n";

                                   server::request_factory req_factory;
                                   std::unique_ptr<server::abstract_request> request =
                                       req_factory.build_request(request_container);
                                   if (!request)
                                   {
                                       // No request parsed, return error
                                       graphs::ResponseContainer response;
                                       response.set_status(graphs::ResponseContainer::ERROR);
                                       respond(response);
                                       return;
                                   }

                                   // Call handler with parsed request and wait for response
                                   std::unique_ptr<server::abstract_response> response =
                                       server::HandlerProxy().handle(std::move(request));

                                   // Build and send protobuffer from abstract_response object
                                   server::response_factory res_factory;
                                   respond(*(res_factory.build_response(response)));
                               }
                               else
                               {
                                   std::cout << "Error parsing proto message\n";
                                   graphs::ResponseContainer response;
                                   response.set_status(graphs::ResponseContainer::ERROR);
                                   respond(response);
                               }
                           });
                   }
                   catch (std::bad_alloc &)
                   {
                       std::cout << "Bad alloc error\n";
                       graphs::ResponseContainer response;
                       response.set_status(graphs::ResponseContainer::ERROR);
                       respond(response);
                   }
               });
}

void connection::respond(graphs::ResponseContainer &container)
{
    namespace io = boost::iostreams;

    // Generate uncompressed buffer from protobuf container
    std::vector<char> uncompressed;
    uncompressed.resize(container.ByteSizeLong());
    container.SerializeToArray(uncompressed.data(), uncompressed.size());

    // Boost gzip compression
    io::filtering_streambuf<io::input> in_str_buf;
    in_str_buf.push(io::gzip_compressor{});
    in_str_buf.push(io::array_source{uncompressed.data(), uncompressed.size()});

    // Reserve size for the length of compressed message
    // Reuse connections internal buffer because its content is no longer used
    m_buffer.resize(LENGHT_FIELD_SIZE);

    // Copy compressed data into the compressed buffer
    std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {}, std::back_inserter(m_buffer));

    // Copy the length of the message in the first 8 bytes of the compressed buffer
    uint64_t len = boost::endian::native_to_big(m_buffer.size() - LENGHT_FIELD_SIZE);
    std::copy(&len, &len + 1, reinterpret_cast<uint64_t *>(m_buffer.data()));

    write(m_buffer.data(), m_buffer.size());
}

void connection::write(const char *buffer, size_t length)
{
    m_sock.async_send(boost::asio::buffer(buffer, length),
                      [this, buffer, length](error_code ec, size_t bytes) {
                          if (ec)
                          {
                              std::cout << "SENDING_ERROR: " << ec << '\n';
                              m_handler.remove(m_identifier);
                          }
                          else
                          {
                              std::cout << "Sent " << bytes << " bytes\n";
                              if (bytes < length)
                                  write(buffer + bytes, length - bytes);
                              else
                                  m_handler.remove(m_identifier);
                          }
                      });
}

}  // namespace server
