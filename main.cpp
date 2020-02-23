#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cstdint>

#include <evhttp.h>

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include "logger/logger.hpp"
#include "thread_pool/thread_pool.hpp"

using boost::asio::ip::tcp;

const int max_length = 1024;
typedef boost::shared_ptr<tcp::socket> socket_ptr;

void handle_request(evhttp_request *req) {
  // only GET and HEAD requests allowed
  if (evhttp_request_get_command(req) != EVHTTP_REQ_GET &&
    evhttp_request_get_command(req) != EVHTTP_REQ_HEAD) {
    evhttp_send_error(req, HTTP_BADREQUEST, 0);
    return;
  }
  // logic with files
  
  auto *OutBuf = evhttp_request_get_output_buffer(req);
  if (!OutBuf)
    return;
  evbuffer_add_printf(OutBuf, "<html><body><center><h1>Hello Wotld!</h1></center></body></html>");
  evhttp_send_reply(req, HTTP_OK, "", OutBuf);
  
  BOOST_LOG_TRIVIAL(info) << "Job have done by thread: " << std::this_thread::get_id() << std::endl;
}

void (*on_request)(evhttp_request* req, void* tp) = [] (evhttp_request *req, void* tp)
{
  BOOST_LOG_TRIVIAL(info) << "New client has come with uri" << req->uri << std::endl;
  static_cast<thread_pool*>(tp)->enqueue(handle_request, req);
};

int main()
{
    thread_pool tp;
    tp.start(3);

    logger::init_log(true, false);

    if (!event_init())
    {
      std::cerr << "Failed to init libevent." << std::endl;
      return EXIT_FAILURE;
    }

    char const SrvAddress[] = "127.0.0.1";
    std::uint16_t SrvPort = 5555;

    std::unique_ptr<evhttp, decltype(&evhttp_free)> Server(evhttp_start(SrvAddress, SrvPort), &evhttp_free);
    if (!Server)
    {
      std::cerr << "Failed to init http server." << std::endl;
      return EXIT_FAILURE;
    }

    evhttp_set_gencb(Server.get(), on_request, &tp);
    
    if (event_dispatch() == EXIT_FAILURE)
    {
      std::cerr << "Failed to run message loop." << std::endl;
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
