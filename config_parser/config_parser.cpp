#include "config_parser.hpp"

#include <fstream>
#include <thread>

#include "../logger/logger.hpp"

std::string ConfigParser::document_root = "/var/www/html";
size_t ConfigParser::cpu_limit = std::thread::hardware_concurrency();
size_t ConfigParser::thread_limit = 256;
std::string ConfigParser::host = "127.0.0.1";
size_t ConfigParser::port = 80;

ConfigParser::conf ConfigParser::parse(const std::string& conf_path) {
    std::ifstream conf_file(conf_path);
    std::string line;

    while (!conf_file.eof()) {
        std::getline(conf_file, line, '\n');

        if (line.empty()) {
            break;
        }

        size_t space_position = line.find_first_of(' ');

        if (line.substr(0, space_position) == "cpu_limit") {
            const auto cpu_count = stoi(line.substr(space_position + 1, line.size() - space_position));
            if (cpu_count > cpu_limit) {
                BOOST_LOG_TRIVIAL(warning) << "cpu_limit more than hardware_concurrency=" << cpu_limit;
                cpu_limit = cpu_count;
            }
        }

        if (line.substr(0, space_position) == "thread_limit") {
            thread_limit = stoi(line.substr(space_position + 1, line.size() - space_position));
        }

        if (line.substr(0, space_position) == "document_root") {
            document_root = line.substr(space_position + 1, line.size() - space_position);
        }

        if (line.substr(0, space_position) == "host") {
            host = line.substr(space_position + 1, line.size() - space_position);
        }

        if (line.substr(0, space_position) == "port") {
            port = stoi(line.substr(space_position + 1, line.size() - space_position));
        }
    }
    
    conf c;
    c.cpu_limit = cpu_limit;
    c.document_root = document_root;
    c.thread_limit = thread_limit;
    c.host = host;
    c.port = port;
    return c;
}