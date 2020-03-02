#include "logger/logger.hpp"
#include "server/server.hpp"
#include "config_parser/config_parser.hpp"


int main(int argc, char** argv) {
  logger::init_log(true, false);
  if (argc > 1) {
    BOOST_LOG_TRIVIAL(info) << "Configure file is in " << argv[1];
    auto conf = ConfigParser::parse(argv[1]);
    BOOST_LOG_TRIVIAL(info) << "DOCUMENT_ROOT: " << conf.document_root;

    server s(conf.host, conf.port, conf.cpu_limit - 1, conf.document_root);
    return s.run();
  } else {
    BOOST_LOG_TRIVIAL(error) << "No path to configure file";
    return EXIT_FAILURE;
  }  
}
