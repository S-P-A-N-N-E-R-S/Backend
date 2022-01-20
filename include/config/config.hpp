#pragma once

#include <boost/program_options.hpp>

namespace server {

namespace config_options {

    const char *const HELP = "help";
    const char *const CONFIG_FILE = "config-file";
    const char *const SERVER_PORT = "server-port";
    const char *const DB_HOST = "db-host";
    const char *const DB_PORT = "db-port";
    const char *const DB_USER = "db-user";
    const char *const DB_NAME = "db-name";
    const char *const DB_PASSWORD = "db-password";
    const char *const DB_TIMEOUT = "db-timeout";
    const char *const SCHEDULER_EXEC_PATH = "scheduler-exec-path";
    const char *const SCHEDULER_PROCESS_LIMIT = "scheduler-process-limit";
    const char *const SCHEDULER_TIME_LIMIT = "scheduler-time-limit";
    const char *const SCHEDULER_RESOURCE_LIMIT = "scheduler-resource-limit";
    const char *const SCHEDULER_SLEEP = "scheduler-sleep";
    const char *const TLS_CERT_PATH = "tls-cert-path";
    const char *const TLS_KEY_PATH = "tls-key-path";

}  // namespace config_options

namespace config_env_vars {

    const char *const XDG_CONFIG_HOME = "XDG_CONFIG_HOME";
    const char *const SERVER_PORT = "SPANNERS_SERVER_PORT";
    const char *const DB_HOST = "SPANNERS_DB_HOST";
    const char *const DB_PORT = "SPANNERS_DB_PORT";
    const char *const DB_USER = "SPANNERS_DB_USER";
    const char *const DB_NAME = "SPANNERS_DB_NAME";
    const char *const DB_PASSWORD = "SPANNERS_DB_PASSWORD";
    const char *const DB_TIMEOUT = "SPANNERS_DB_TIMEOUT";
    const char *const SCHEDULER_EXEC_PATH = "SPANNERS_SCHEDULER_EXEC_PATH";
    const char *const SCHEDULER_PROCESS_LIMIT = "SPANNERS_SCHEDULER_PROCESS_LIMIT";
    const char *const SCHEDULER_TIME_LIMIT = "SPANNERS_SCHEDULER_TIME_LIMIT";
    const char *const SCHEDULER_RESOURCE_LIMIT = "SPANNERS_SCHEDULER_RESOURCE_LIMIT";
    const char *const SCHEDULER_SLEEP = "SPANNERS_SCHEDULER_SLEEP";
    const char *const TLS_CERT_PATH = "SPANNERS_TLS_CERT_PATH";
    const char *const TLS_KEY_PATH = "SPANNERS_TLS_KEY_PATH";

}  // namespace config_env_vars

class config_parser
{
public:
    /**
     * @brief Get singleton parser instance
     */
    static config_parser &instance();

    /**
     * @brief Parses the commandline options, environment variables and config file. The mentioned order indicates
     * the prioritization, so that command line options are always preferred.
     *
     * @param argc Count of commandline arguments
     * @param argv Array of commandline arguments
     */
    void parse(int argc, const char **argv);

    friend const boost::program_options::variables_map &config();
    friend const boost::program_options::variable_value &config(const char *option);

    friend const std::string &get_db_connection_string();

private:
    boost::program_options::variables_map m_config_map;

    boost::program_options::options_description m_generic_options;
    boost::program_options::options_description m_configuration_options;
    boost::program_options::options_description m_cmdline_options;

    const std::string m_default_config_home;
    const std::string m_rel_config_file_path;

    config_parser();

    void set_options();

    void get_config_file_path(std::string &config_file_path);
    void create_default_config_file(const std::string &config_file_path);

    void parse_command_line(int argc, const char **argv);
    void parse_environment_variables();
    void parse_config_file();

    void process_generic_options();

    config_parser(const config_parser &) = delete;
    config_parser(config_parser &&) = delete;
    config_parser &operator=(const config_parser &) = delete;
    config_parser &operator=(config_parser &&) = delete;

};  // class config_parser

/**
 * @brief Gets the parsed config options
 *
 * @return Map of key value option pairs. Key represents the option name and value the corresponding value.
 */
const boost::program_options::variables_map &config();

/**
 * @brief Gets a single option
 *
 * @param name Name of option
 * @return Value of option
 */
const boost::program_options::variable_value &config(const char *option);

/**
 * @brief Constructs the database connection string from the parsed option values
 *
 * @return database connection string
 */
const std::string &get_db_connection_string();

}  // namespace server
