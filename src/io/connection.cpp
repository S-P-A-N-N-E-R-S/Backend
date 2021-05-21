#include <networking/io/connection.hpp>

#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <algorithm>
#include <boost/asio/completion_condition.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <iostream>
#include <iterator>
#include <string>

#include <networking/io/connection_handler.hpp>

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
                       std::cout << "ERROR\n";

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
                                   std::cout << "ERROR!!!\n";

                               // Boost gzip decompression
                               io::filtering_streambuf<io::input> in_str_buf;
                               in_str_buf.push(io::gzip_decompressor{});
                               in_str_buf.push(io::array_source{m_buffer.data(), m_buffer.size()});

                               // Copy compressed data into the decompressed buffer
                               std::vector<char> decompressed;
                               decompressed.reserve(m_buffer.size());
                               std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {},
                                         std::back_inserter(decompressed));

                               graphs::RequestContainer request;
                               if (request.ParseFromArray(decompressed.data(), decompressed.size()))
                               {
                                   std::cout << "Parsed proto message!\n";

                                   // TODO Work with proto obj here

                                   graphs::ResponseContainer response;
                                   // TODO Dont use ERRO here when the protocol is fixed
                                   response.set_status(graphs::ResponseContainer::ERROR);
                                   write(response);
                               }
                               else
                               {
                                   std::cout << "Error parsing proto message\n";
                                   graphs::ResponseContainer response;
                                   response.set_status(graphs::ResponseContainer::ERROR);
                                   write(response);
                               }
                           });
                   }
                   catch (std::bad_alloc &)
                   {
                       std::cout << "Bad alloc error\n";
                       graphs::ResponseContainer response;
                       response.set_status(graphs::ResponseContainer::ERROR);
                       write(response);
                   }
               });
}

void connection::write(graphs::ResponseContainer &container)
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

    std::vector<char> compressed;
    compressed.resize(LENGHT_FIELD_SIZE);  // Reserve size for the length of compressed message

    // Copy compressed data into the compressed buffer
    std::copy(std::istreambuf_iterator<char>{&in_str_buf}, {}, std::back_inserter(compressed));

    // Copy the length of the message in the first 8 bytes of the compressed buffer
    uint64_t len = boost::endian::native_to_big(compressed.size() - LENGHT_FIELD_SIZE);
    std::copy(&len, &len + 1, reinterpret_cast<uint64_t *>(compressed.data()));

    m_sock.async_send(boost::asio::buffer(compressed.data(), compressed.size()),
                      [this](error_code ec, size_t bytes) {
                          if (ec)
                          {
                              std::cout << "SENDING_ERROR: " << ec << '\n';
                          }
                          else
                          {
                              std::cout << "Sent " << bytes << " bytes\n";
                          }

                          // Remove connection from connection_handler when response is sent
                          m_handler.remove(m_identifier);
                      });
}

}  // namespace server
