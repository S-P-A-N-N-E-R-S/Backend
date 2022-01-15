#ifndef IO_SERVER_REQUEST_HANDLING_HPP
#define IO_SERVER_REQUEST_HANDLING_HPP

#include <vector>

#include <networking/messages/meta_data.hpp>
#include <persistence/database_wrapper.hpp>
#include <persistence/user.hpp>

#include "container.pb.h"
#include "meta.pb.h"

namespace server {

namespace request_handling {

    /**
     * @brief handled_request Using declaration for the result of a handled request that responsd
     * with a <graphs::ResponseContainer>
     */
    using handled_request = std::pair<meta_data, graphs::ResponseContainer>;

    /**
     * @brief Creates response to requests for all available handlers
     *
     * @return handled_request containing the meta data and the response
     */
    handled_request handle_available_handlers();

    /**
     * @brief Creates response to requests asking for job status data
     *
     * @param db Reference to a database connection to get the status information from
     * @param user Constant reference to a user struct to only query for one users jobs
     * @return handled_request containing the meta data and the response
     */
    handled_request handle_status(database_wrapper &db, const user &user);

    /**
     * @brief Creates response to requests for job results
     *
     * @param db Reference to a database connection to get the status information from
     * @param meta Constant reference to the requests meta data
     * @param request Constant reference to the data container of the handled request
     * @param user Constant reference to a user struct to only query for one users jobs
     * @return handled_request containing the meta data and the response
     */
    std::pair<meta_data, binary_data> handle_result(database_wrapper &db,
                                                    const graphs::MetaData &meta,
                                                    const graphs::RequestContainer &request,
                                                    const user &user);

    /**
     * @brief Creates response for results asking to create a new job
     *
     * @param db Reference to a database connection to get the status information from
     * @param meta Constant reference to the requests meta data
     * @param buffer Constant reference to the buffer containing the compressed data of the job
     * @param user Constant reference to a user struct to only query for one users jobs
     * @return handled_request containing the meta data and the response
     */
    handled_request handle_new_job(database_wrapper &db, const graphs::MetaData &meta,
                                   const std::vector<char> &buffer, const user &user);

}  // namespace request_handling

}  // namespace server

#endif
