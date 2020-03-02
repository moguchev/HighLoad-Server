#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser() = default;
    struct conf {
        std::string document_root;
        size_t cpu_limit;
        size_t thread_limit;
        std::string host;
        size_t port;
    };
    static conf parse(const std::string& conf_path);
private:
    static std::string document_root;
    static size_t cpu_limit;
    static size_t thread_limit;
    static std::string host;
    static size_t port;
};

#endif // CONFIG_PARSER_HPP