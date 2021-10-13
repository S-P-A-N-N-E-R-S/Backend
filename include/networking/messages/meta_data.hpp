#ifndef NETWORKING_MESSAGE_META_DATA_HPP
#define NETWORKING_MESSAGE_META_DATA_HPP

#include <string>

#include <meta.pb.h>

namespace server {

/**
 * @brief Struct representing meta data of a request/response
 */
struct meta_data {
    graphs::RequestType request_type;

    std::string handler_type;

    std::string job_name;
};

}  // namespace server

#endif
