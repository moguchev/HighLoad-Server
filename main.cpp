#include "logger/logger.hpp"
#include "server/server.hpp"


int main() {
  logger::init_log(true, false);

  server s("127.0.0.1", 8080, 3, "/home/leo/GitHub/http-test-suite");
  
  return s.run();
}
