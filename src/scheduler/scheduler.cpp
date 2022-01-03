#include <boost/filesystem.hpp>
#include <scheduler/process_flags.hpp>
#include <scheduler/scheduler.hpp>
#include <stdexcept>
#include <thread>

namespace server {

scheduler &scheduler::instance()
{
    static scheduler instance =
        scheduler("./src/handler_process", 4,
                  "host=localhost port=5432 user= spanner_user dbname=spanner_db "
                  "password=pwd connect_timeout=10");
    return instance;
}

scheduler::scheduler(const std::string &exec_path, size_t process_limit,
                     const std::string &database_connection)
    : m_exec_path(exec_path)
    , m_process_limit(process_limit)
    , m_time_limit(0)
    , m_resource_limit(0)
    , m_processes(0)
    , m_database_connection_string(database_connection)
    , m_database(database_connection)
    , m_thread_started(false)
    , m_thread_halted(false)
    , m_stop(false)
    , m_sleep(std::chrono::milliseconds(1000))
    , m_thread()
{
    const boost::filesystem::path path{m_exec_path};
    if (!boost::filesystem::exists(path))
    {
        throw std::invalid_argument("Path " + exec_path + " does not exist!");
    }
    else if (!(boost::filesystem::is_regular_file(m_exec_path) ||
               boost::filesystem::is_symlink(m_exec_path)))
    {
        throw std::invalid_argument("Path " + exec_path + " is not a regular file!");
    }
}

scheduler::~scheduler()
{
    stop_scheduler(true);
    m_thread.join();
}

void scheduler::set_time_limit(int64_t time_limit)
{
    std::lock_guard<std::mutex> lock_g(m_mutex);

    m_time_limit = time_limit;
}

void scheduler::set_resource_limit(rlim64_t resource_limit)
{
    std::lock_guard<std::mutex> lock_g(m_mutex);

    m_resource_limit = resource_limit;
}

void scheduler::set_process_limit(size_t process_limit)
{
    std::lock_guard<std::mutex> lock_g(m_mutex);

    m_process_limit = process_limit;
}

void scheduler::set_sleep(int64_t sleep)
{
    std::lock_guard<std::mutex> lock_g(m_mutex);

    m_sleep = std::chrono::milliseconds(sleep);
}

void scheduler::start()
{
    // we want to allow a call to start() only once
    if (m_thread_started)
    {
        throw std::runtime_error("scheduler::start() called more than once!");
    }
    m_thread_started = true;
    m_thread = std::thread([this] {
        this->run_thread();
    });
}

bool scheduler::running()
{
    return m_thread_started && (!m_thread_halted);
}

void scheduler::run_thread()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    lock.unlock();

    while (true)
    {
        lock.lock();

        if (m_stop && m_processes.size() == 0)
        {
            break;
        }

        // First: Review processes, stop if above time limit and update database if errors ocurred (otherwise handler has written success to the database)
        for (auto it = m_processes.begin(); it != m_processes.end();)
        {
            if (*it == nullptr)
            {
                throw std::runtime_error("scheduler: A task is nullpointer but should not be!");
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - (*it)->start)
                               .count();

            if (!(*it)->process->running())
            {
                (*it)->ios.run();

                switch ((*it)->process->exit_code())
                {
                    case process_flags::SUCCESS: {
                        // Do nothing, in this case handling already updated database
                        m_database.set_finished((*it)->job_id, graphs::StatusType::SUCCESS,
                                                (*it)->data_cout.get(), (*it)->data_cerr.get());
                    }
                    break;

                    case process_flags::SEGFAULT: {
                        m_database.set_finished((*it)->job_id, graphs::StatusType::FAILED,
                                                (*it)->data_cout.get(), "Segfault");
                    }
                    break;

                    default: {
                        m_database.set_finished((*it)->job_id, graphs::StatusType::FAILED,
                                                (*it)->data_cout.get(), (*it)->data_cerr.get());
                    }
                    break;
                }

                it = m_processes.erase(it);
            }
            else if (m_time_limit > 0 && elapsed > m_time_limit)
            {
                (*it)->process->terminate();

                m_database.set_finished((*it)->job_id, graphs::StatusType::ABORTED, "", "Timeout");

                it = m_processes.erase(it);
            }
            else
            {
                it++;
            }
        }

        // Second: spawn new tasks if possible
        if (m_processes.size() < m_process_limit && !m_stop)
        {
            auto new_jobs = m_database.get_next_jobs(m_process_limit - m_processes.size());

            for (const auto &job_info : new_jobs)
            {
                m_database.set_started(job_info.first);
                auto process =
                    std::unique_ptr<job_process>(new job_process{job_info.first, job_info.second});
                process->start = std::chrono::steady_clock::now();
                process->process = std::make_unique<boost::process::child>(
                    m_exec_path,
                    std::to_string(job_info.first),   //task_id
                    std::to_string(job_info.second),  //user_id
                    m_database_connection_string,
                    std::to_string(m_resource_limit),  //memory limit
                    boost::process::std_in.close(), boost::process::std_out > process->data_cout,
                    boost::process::std_err > process->data_cerr, process->ios);

                m_processes.insert(std::move(process));
            }
        }

        auto tmp_sleep = m_sleep;

        lock.unlock();

        std::this_thread::sleep_for(tmp_sleep);
    }
    m_thread_halted = true;
}

void scheduler::stop_scheduler(bool force)
{
    std::lock_guard<std::mutex> lock_g(m_mutex);
    m_stop = true;
    if (force)
    {
        for (auto &p : m_processes)
        {
            if (p->process->running())
            {
                p->process->terminate();
            }

            m_database.set_finished(p->job_id, graphs::StatusType::ABORTED, "",
                                    "Global scheduler stop");
        }
        m_processes.clear();
    }
}

void scheduler::cancel_job(int job_id, int user_id)
{
    std::lock_guard<std::mutex> lock_g(m_mutex);

    for (auto it = m_processes.begin(); it != m_processes.end(); it++)
    {
        if ((*it)->job_id == job_id && (*it)->user_id == user_id)
        {
            if ((*it)->process->running())
            {
                (*it)->process->terminate();

                m_database.set_finished((*it)->job_id, graphs::StatusType::ABORTED, "",
                                        "Aborted by Request");

                m_processes.erase(it);
            }
            //If it is not running anymore, we do not need to abort it. Exit status will be checked next time during run_thread
            return;
        }
    }

    m_database.set_finished(job_id, graphs::StatusType::ABORTED, "", "Preemptive abort");
}

void scheduler::cancel_user_jobs(int user_id)
{
    std::lock_guard<std::mutex> lock_g(m_mutex);

    for (auto it = m_processes.begin(); it != m_processes.end();)
    {
        if ((*it)->user_id == user_id)
        {
            if ((*it)->process->running())
            {
                (*it)->process->terminate();
            }
            m_processes.erase(it);
        }
        else
        {
            it++;
        }
    }
}

}  // namespace server
