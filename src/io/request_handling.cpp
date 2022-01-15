#include <networking/io/request_handling.hpp>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include <handling/handler_utilities.hpp>
#include <networking/responses/new_job_response.hpp>
#include <networking/responses/response_factory.hpp>
#include <networking/responses/status_response.hpp>

#include "new_job_response.pb.h"
#include "result.pb.h"

using graphs::MetaData;
using graphs::NewJobResponse;
using graphs::RequestContainer;
using graphs::RequestType;
using graphs::ResponseContainer;
using graphs::ResultRequest;
using graphs::StatusResponse;

namespace server {

namespace request_handling {

    handled_request handle_available_handlers()
    {
        return handled_request{meta_data{RequestType::AVAILABLE_HANDLERS},
                               response_factory::build_response(available_handlers())};
    }

    handled_request handle_status(database_wrapper &db, const user &user)
    {
        std::vector<job_entry> jobs = db.get_job_entries(user.user_id);

        StatusResponse status_response;
        auto respStates = status_response.mutable_states();

        for (const auto &job : jobs)
        {
            respStates->Add(db.get_status_data(job.job_id, user.user_id));
        }

        auto response =
            std::make_unique<server::status_response>(std::move(status_response), status_code::OK);

        return handled_request{meta_data{RequestType::STATUS},
                               response_factory::build_response(std::move(response))};
    }

    std::pair<meta_data, binary_data> handle_result(database_wrapper &db, const MetaData &meta,
                                                    const RequestContainer &request,
                                                    const user &user)
    {
        ResultRequest res_req;
        if (const bool ok = request.request().UnpackTo(&res_req); !ok)
        {
            throw ResponseContainer::INVALID_REQUEST_ERROR;
        }

        const int job_id = res_req.jobid();

        meta_data job_meta_data = db.get_meta_data(job_id, user.user_id);
        auto [type, binary_response] = db.get_response_data_raw(job_id, user.user_id);

        // Add latest status information to the algorithm response
        ResponseContainer status_container;
        *(status_container.mutable_statusdata()) = db.get_status_data(job_id, user.user_id);
        size_t old_len = binary_response.size();
        binary_response.resize(old_len + status_container.ByteSizeLong());
        status_container.SerializeToArray(binary_response.data() + old_len, binary_response.size());

        return std::pair{job_meta_data, binary_response};
    }

    handled_request handle_new_job(database_wrapper &db, const MetaData &meta,
                                   const std::vector<char> &buffer, const user &user)
    {
        namespace io = boost::iostreams;

        io::filtering_streambuf<io::input> in_str_buf;
        in_str_buf.push(io::gzip_decompressor{});
        in_str_buf.push(io::array_source{buffer.data(), buffer.size()});

        // Copy compressed data into the decompressed buffer
        std::vector<char> decompressed;
        decompressed.reserve(buffer.size());
        decompressed.assign(std::istreambuf_iterator<char>{&in_str_buf}, {});
        binary_data_view binary(reinterpret_cast<std::byte *>(decompressed.data()),
                                decompressed.size());

        int job_id = db.add_job(user.user_id,
                                meta_data{meta.type(), meta.handlertype(), meta.jobname()}, binary);

        NewJobResponse new_job_resp;
        new_job_resp.set_jobid(job_id);

        auto response =
            std::make_unique<new_job_response>(std::move(new_job_resp), status_code::OK);

        return handled_request{meta_data{RequestType::NEW_JOB_RESPONSE},
                               response_factory::build_response(std::move(response))};
    }

}  // namespace request_handling

}  // namespace server
