#include "logger/logger.hpp"
#include "server/server.hpp"


int main()
{
  logger::init_log(true, false);

  server s("127.0.0.1", 3000, 4, "/var/www/html");
  
  return s.run();
}
