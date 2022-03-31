#pragma once

#include <sys/resource.h>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <persistence/database_wrapper.hpp>
#include <unordered_set>

namespace server {

using time_point = std::chrono::steady_clock::time_point;

/**
 * @brief Stores information of a running process
 * 
 */
struct job_process {
    /**
     * @brief Id of the job the process handles
     * 
     */
    int job_id;
    /**
     * @brief Id of the user the job belongs to
     * 
     */
    int user_id;
    /**
     * @brief boost::asio::io_service to catch output
     * 
     */
    boost::asio::io_service ios;
    /**
     * @brief Stores cout output
     * 
     */
    std::future<std::string> data_cout;
    /**
     * @brief Stores cerr output
     * 
     */
    std::future<std::string> data_cerr;
    /**
     * @brief Pointer to the process itself
     * 
     */
    std::unique_ptr<boost::process::child> process;
    /**
     * @brief Starting time of the process
     * 
     */
    time_point start;
};

class scheduler
{
public:
    static scheduler &instance();

    /**
     * @brief Destroy the scheduler object. Internally calls stop_scheduler(true).
     */
    ~scheduler();

    /**
     * @brief Set the time limit in milliseconds. Can be used while scheduler is active.
     * In this case, every process above time limit is cancelled.
     *
     * @param time_limit If > 0,  this sets the time limit in milliseconds. Otherwise, no
     * time limit is enforced (and possible prior limit is removed)
     */
    void set_time_limit(int64_t time_limit);

    /**
     * @brief Get the current time limit in milliseconds
     *
     * @return m_time_limit int64_t
     */
    int64_t get_time_limit() const;

    /**
     * @brief Set the memory resource limit in bytes. Can be used while scheduler is active. 
     * In this case, every new process will obey the limit.
     *
     * @param ressource_limit If > 0, this sets the memory resource limit in bytes. Otherwise,
     * no resource limit is enforced (and possible prior limit is removed)
     */
    void set_resource_limit(rlim64_t resource_limit);

    /**
     * @brief Get the current resource limit in milliseconds
     *
     * @return m_resource_limit rlim64_t
     */
    rlim64_t get_resource_limit() const;

    /**
     * @brief Set the maximum number of processes. Can be used while scheduler is active.
     * In this case, no new processes are spawned until slots are free.
     *
     * @param process_limit Number of processes to schedule parallel
     */
    void set_process_limit(size_t process_limit);

    /**
     * @brief Get the current process limit in milliseconds
     *
     * @return m_process_limit size_t
     */
    size_t get_process_limit() const;

    /**
     * @brief Set the waiting intervall between scheduling jobs to minimize while(true) overhead.
     *
     * @param sleep Time to sleep between two scheduling actions in milliseconds.
     */
    void set_sleep(int64_t sleep);

    /**
     * @brief Get the current sleep intervall in milliseconds
     *
     * @return m_sleep int64_t
     */
    int64_t get_sleep() const;

    /**
     * @brief Starts the scheduler
     */
    void start();

    /**
     * @brief Checks if scheduler is still running
     *
     * @return true if scheduler runs
     */
    bool running();

    /**
     * @brief Stops the scheduler.
     *
     * @param force true: all running jobs are cancelled. false: running jobs are allowed
     * to exit normally
     */
    void stop_scheduler(bool force);

    /**
     * @brief Stops execution of a process. If it is already running, terminate it. If not,
     * change status in database to aborted so that it never gets scheduled
     */
    void cancel_job(int job_id, int user_id);

    /**
     * @brief Cancels all running jobs of the user with id user_id. No updates are written into the
     * database.
     *
     * @param user_id
     */
    void cancel_user_jobs(int user_id);

private:
    scheduler(const std::string &exec_path, size_t process_limit, int64_t time_limit,
              rlim64_t resource_limit, const std::string &database_connection);

    std::string m_exec_path;
    size_t m_process_limit;
    int64_t m_time_limit;
    rlim64_t m_resource_limit;

    mutable std::mutex m_mutex;
    std::unordered_set<std::unique_ptr<job_process>> m_processes;

    std::string m_database_connection_string;
    database_wrapper m_database;

    bool m_thread_started;
    bool m_thread_halted;
    bool m_stop;

    std::chrono::milliseconds m_sleep;

    std::thread m_thread;

    void run_thread();

    //Rule of five

    scheduler(const scheduler &) = delete;
    scheduler(scheduler &&) = delete;
    scheduler &operator=(const scheduler &) = delete;
    scheduler &operator=(scheduler &&) = delete;

};  // class scheduler

}  //namespace server
