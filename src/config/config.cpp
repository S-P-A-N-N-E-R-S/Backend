#include <boost/make_shared.hpp>
#include <config/config.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

using boost::program_options::variable_value;
using boost::program_options::variables_map;

namespace server {

config_parser &config_parser::instance()
{
    static config_parser instance;
    return instance;
}

void config_parser::parse(int argc, const char **argv)
{
    set_options();
    // Already parsed option values will not be changed, so that command line options has always precedence.
    parse_command_line(argc, argv);
    parse_environment_variables();
    parse_config_file();
    process_generic_options();
}

config_parser::config_parser()
    : m_config_map()
    , m_generic_options("Generic options")
    , m_configuration_options("Configuration")
    , m_cmdline_options("Allowed options")
    , m_default_config_home(std::string(std::getenv("HOME")) + "/.config")
    , m_rel_config_file_path("/spanners/server.cfg")
{
}

void config_parser::set_options()
{
    using namespace boost::program_options;

    // clang-format off
    m_generic_options.add_options()
        (config_options::HELP, "show help message")
        (config_options::CONFIG_FILE, value<std::string>(),"absolute path to configuration file "
        "(preferred to config file located in config home directory)");
    // clang-format on

    // Add definitions for "business logic" variables
    {
        const auto add = [this](const char *key, auto def_val, const char *description) {
            this->m_configuration_options.add(boost::make_shared<option_description>(
                key, value<decltype(def_val)>()->default_value(std::move(def_val)), description));
        };

        add(config_options::SERVER_PORT, static_cast<unsigned short>(4711), "server port");
        add(config_options::DB_HOST, std::string{"localhost"}, "database host");
        add(config_options::DB_PORT, 5432, "database port");
        add(config_options::DB_USER, std::string{"spanner_user"}, "database user");
        add(config_options::DB_NAME, std::string{"spanner_db"}, "database name");
        add(config_options::DB_PASSWORD, std::string{"pwd"}, "database password");
        add(config_options::DB_TIMEOUT, 10, "database timeout in seconds");
        add(config_options::SCHEDULER_EXEC_PATH, std::string{"./src/handler_process"},
            "absolute path to the handler_process executable");
        add(config_options::SCHEDULER_PROCESS_LIMIT, size_t{4},
            "maximum number of concurrent worker processes allowed by the scheduler");
        add(config_options::SCHEDULER_TIME_LIMIT, int64_t{0},
            "scheduler time limit in milliseconds");
        add(config_options::SCHEDULER_RESOURCE_LIMIT, int64_t{0},
            "scheduler resource limit in bytes (if zero or negative, no resource limits are "
            "enforced)");
        add(config_options::SCHEDULER_SLEEP, int64_t{1000}, "scheduler sleep in milliseconds");
        add(config_options::TLS_CERT_PATH, std::string{}, "path to signed TLS certificate");
        add(config_options::TLS_KEY_PATH, std::string{}, "path to key file");
    }

    m_cmdline_options.add(m_generic_options).add(m_configuration_options);
}

void config_parser::get_config_file_path(std::string &config_file_path)
{
    // Prefer explicitly specified config file
    if (m_config_map.count(config_options::CONFIG_FILE))
    {
        config_file_path = m_config_map[config_options::CONFIG_FILE].as<std::string>();
    }
    else if (const char *xdg_config_home = std::getenv(config_env_vars::XDG_CONFIG_HOME))
    {
        config_file_path = xdg_config_home + m_rel_config_file_path;
    }
    else
    {
        // Use default config home if $XDG_CONFIG_HOME is not set
        config_file_path = m_default_config_home + m_rel_config_file_path;
    }
}

void config_parser::create_default_config_file(const std::string &config_file_path)
{
    const std::string default_config("server-port=4711\n"
                                     "\n"
                                     "db-host=localhost\n"
                                     "# db-port=5432\n"
                                     "db-user=spanner_user\n"
                                     "db-name=spanner_db\n"
                                     "db-password=pwd\n"
                                     "db-timeout=10\n"
                                     "\n"
                                     "# tls-cert-path=/cert-path\n"
                                     "# tls-key-path=/key-path");

    // Create config sub directory
    const std::string config_dir = std::filesystem::path(config_file_path).remove_filename();
    if (!std::filesystem::exists(config_dir))
    {
        std::filesystem::create_directories(config_dir);
        std::filesystem::permissions(config_dir, std::filesystem::perms::owner_all);
    }

    // Create file
    std::ofstream ofstream(config_file_path);
    if (!ofstream)
    {
        std::cerr << "[ERROR] Can not create default config file: " << config_file_path
                  << std::endl;
    }
    else
    {
        ofstream << default_config;
        std::cout << "[SERVER] Created default config file: " << config_file_path << std::endl;
    }
}

void config_parser::parse_command_line(int argc, const char **argv)
{
    store(boost::program_options::command_line_parser(argc, argv).options(m_cmdline_options).run(),
          m_config_map);
    notify(m_config_map);
}

void config_parser::parse_environment_variables()
{
    store(boost::program_options::parse_environment(
              m_configuration_options,
              [](const std::string &env_var) {
                  static const std::unordered_map<std::string, std::string> env_to_cfg{
                      {config_env_vars::SERVER_PORT, config_options::SERVER_PORT},
                      {config_env_vars::DB_HOST, config_options::DB_HOST},
                      {config_env_vars::DB_PORT, config_options::DB_PORT},
                      {config_env_vars::DB_USER, config_options::DB_USER},
                      {config_env_vars::DB_NAME, config_options::DB_NAME},
                      {config_env_vars::DB_PASSWORD, config_options::DB_PASSWORD},
                      {config_env_vars::DB_TIMEOUT, config_options::DB_TIMEOUT},
                      {config_env_vars::SCHEDULER_EXEC_PATH, config_options::SCHEDULER_EXEC_PATH},
                      {config_env_vars::SCHEDULER_PROCESS_LIMIT,
                       config_options::SCHEDULER_PROCESS_LIMIT},
                      {config_env_vars::SCHEDULER_TIME_LIMIT, config_options::SCHEDULER_TIME_LIMIT},
                      {config_env_vars::SCHEDULER_RESOURCE_LIMIT,
                       config_options::SCHEDULER_RESOURCE_LIMIT},
                      {config_env_vars::SCHEDULER_SLEEP, config_options::SCHEDULER_SLEEP},
                      {config_env_vars::TLS_CERT_PATH, config_options::TLS_CERT_PATH},
                      {config_env_vars::TLS_CERT_PATH, config_options::TLS_KEY_PATH}};

                  if (auto it = env_to_cfg.find(env_var); it != env_to_cfg.end())
                  {
                      return it->second;
                  }

                  return std::string{};
              }),
          m_config_map);
    notify(m_config_map);
}

void config_parser::parse_config_file()
{
    std::string config_file_path;
    get_config_file_path(config_file_path);

    // Create default config file if not exist
    if (!std::filesystem::exists(config_file_path))
    {
        create_default_config_file(config_file_path);
    }

    // Read config file
    std::ifstream ifs(config_file_path);
    if (!ifs)
    {
        std::cerr << "[ERROR] Can not open config file: " << config_file_path << std::endl;
    }
    else
    {
        store(boost::program_options::parse_config_file(ifs, m_configuration_options),
              m_config_map);
        notify(m_config_map);
    }
}

void config_parser::process_generic_options()
{
    if (m_config_map.count(config_options::HELP))
    {
        std::cout << m_cmdline_options << "\n";
        exit(0);
    }
}

const boost::program_options::variables_map &config()
{
    return config_parser::instance().m_config_map;
}

void modify_config(const std::string &key, variable_value new_value)
{
    // This cast is valid because boost::program_options::variables_map inherits from
    // std::map<std::string, boost::program_options::variable_value>
    auto &mutable_config_map = static_cast<std::map<std::string, variable_value> &>(
        config_parser::instance().m_config_map);
    mutable_config_map[key] = std::move(new_value);
}

const boost::program_options::variable_value &config(const char *option)
{
    return config()[option];
}

const std::string &get_db_connection_string()
{
    static std::string db_connection_string(
        "host=" + config(config_options::DB_HOST).as<std::string>() +
        " port=" + std::to_string(config(config_options::DB_PORT).as<int>()) +
        " user=" + config(config_options::DB_USER).as<std::string>() +
        " dbname=" + config(config_options::DB_NAME).as<std::string>() +
        " password=" + config(config_options::DB_PASSWORD).as<std::string>() +
        " connect_timeout=" + std::to_string(config(config_options::DB_TIMEOUT).as<int>()));
    return db_connection_string;
}

}  // namespace server
