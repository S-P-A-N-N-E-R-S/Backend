#include <sys/resource.h>
#include <unistd.h>
#include <handling/handler_utilities.hpp>
#include <iostream>
#include <networking/messages/meta_data.hpp>
#include <persistence/database_wrapper.hpp>
#include <scheduler/process_flags.hpp>
#include <string>
#include <vector>

#include <google/protobuf/util/time_util.h>

using namespace server;

/**
 * @brief
 *
 * @param argc
 * @param argv (job_id, user_id, db_connection_string[, memory limit])
 * @return int
 */
int main(int argc, char *argv[])
{
    //an alternative to using args could be: boost.org/doc/libs/1_76_0/doc/html/interprocess/sharedmemorybetweenprocesses.html
    // Just give on arg as name of shared memory and store other arguments an/or returns there

    if (argc != 5)
    {
        std::cerr << "Wrong number of arguments" << std::endl;
        return process_flags::GENERAL_ERROR;
    }

    int job_id;
    int user_id;

    try
    {
        job_id = std::stoi(argv[1]);
        user_id = std::stoi(argv[2]);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Could not parse job_id/user_id!" << std::endl;
        return process_flags::GENERAL_ERROR;
    }

    rlim64_t ram_limit;
    try
    {
        ram_limit = std::stoull(argv[4]);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Could not parse ram_limit!" << std::endl;
        return process_flags::GENERAL_ERROR;
    }

    if (ram_limit > 0)
    {
        struct rlimit64 old_lim, new_lim;
        if (getrlimit64(RLIMIT_AS, &old_lim) != 0)
        {
            std::cerr << "getrlimit64 failed!" << std::endl;
            return process_flags::GENERAL_ERROR;
        }

        if (ram_limit <= old_lim.rlim_max)
        {
            new_lim.rlim_cur = ram_limit;
            //std::cout << new_lim.rlim_cur << std::endl;
            new_lim.rlim_max = old_lim.rlim_max;
        }
        else
        {
            std::cerr << "rlim_max smaller than ram_limit!" << std::endl;
            return process_flags::GENERAL_ERROR;
        }

        // Warning: This will not work as expected for very small limits because some heap memory is always pre-allocated from the start.
        // However, for every reasonable size this should work. On my system, this is somewhere between 1<<15 and 1<<16
        if (setrlimit64(RLIMIT_AS, &new_lim) == -1)
        {
            std::cerr << "setrlimit64 failed!" << std::endl;
            return process_flags::GENERAL_ERROR;
        }
    }

    database_wrapper database(argv[3]);
    meta_data meta = database.get_meta_data(job_id, user_id);
    auto [type, request] = database.get_request_data(job_id, user_id);

    auto response = server::handle(meta, request);

    database.add_response(job_id, type, response.first, response.second);

    return process_flags::SUCCESS;
}
