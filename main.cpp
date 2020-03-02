#include "logger/logger.hpp"
#include "server/server.hpp"
#include "config_parser/config_parser.hpp"


int main(int argc, char** argv) {
  logger::init_log(true, false);
  if (argc > 1) {
    auto conf = ConfigParser::parse(argv[1]);
    server s(conf.host, conf.port, conf.cpu_limit - 1, conf.document_root);
    return s.run();
  } else {
    BOOST_LOG_TRIVIAL(error) << "expected path to conf in argv";
    return EXIT_FAILURE;
  }  
}
