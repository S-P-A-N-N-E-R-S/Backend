#pragma once

namespace server {
class process_flags
{
    process_flags() = delete;

public:
    static const int SUCCESS = 0;
    //some exit codes are "reserved" for general system errors, for more information visit
    // https://tldp.org/LDP/abs/html/exitcodes.html, https://www.man7.org/linux/man-pages/man7/signal.7.html
    // and look into usr/include/sysexit.h

    static const int GENERAL_ERROR = 1;

    static const int SEGFAULT = 11;  //Example for a reserved exit status signal
};
}  // namespace server