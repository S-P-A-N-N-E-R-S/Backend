#pragma once

namespace server {
/**
 * @brief Stores possible return flags of processes for simpler understanding.
 * The flags depend on the exit code of a process. Some exit codes are "reserved" 
 * for general system errors, for more information visit
 * https://tldp.org/LDP/abs/html/exitcodes.html, 
 * https://www.man7.org/linux/man-pages/man7/signal.7.html,
 * or look into usr/include/sysexit.h
 * 
 */
class process_flags
{
    process_flags() = delete;

public:
    /**
     * @brief Process ended successfull
     * 
     */
    static const int SUCCESS = 0;

    /**
     * @brief Process terminated with a general error
     * 
     */
    static const int GENERAL_ERROR = 1;

    /**
     * @brief Process terminated with segfault
     * 
     */
    static const int SEGFAULT = 11;
};
}  // namespace server