#ifndef SERVER_HPP
#define SERVER_HPP


#include <string>
#include <utility>

#include <evhttp.h>

#include "../logger/logger.hpp"
#include "../thread_pool/thread_pool.hpp"

#include <string>
#include <unordered_map>



const auto MAX_DATE_BUFFER_SIZE = 256;


namespace HTTP {
    namespace HEADER {
        const auto DATE = "Date";
        const auto SERVER = "Server";
        const auto CONNECTION = "Connection";
        const auto CONTENT_TYPE = "Content-type";
        const auto CONTENT_LENGTH = "Content-length";
    } // HEADER

    const auto DATE_FORMAT = "%a, %d %b %Y %H:%M:%S GMT";

    namespace MIME {
        const auto APPLICATION_OCTET_STREAM = "application/octet-stream";
        const auto TEXT_HTML = "text/html";
        const auto TEXT_CSS = "text/css";
        const auto APPLICATION_JAVASCRIPT = "application/javascript";
        const auto IMAGE_JPEG = "image/jpeg";
        const auto IMAGE_PNG = "image/png";
        const auto IMAGE_GIF = "image/gif";
        const auto APPLICATION_X_SHOCKWAVE_FLASH = "application/x-shockwave-flash";
    } // MIME

    namespace EXT {
        const std::string HTML = ".html";
        const std::string CSS = ".css";
        const std::string JS = ".js";
        const std::string JPG = ".jpg";
        const std::string JPEG = ".jpeg";
        const std::string PNG = ".png";
        const std::string GIF = ".gif";
        const std::string SWF = ".swf";
    } // EXT
} // HTTP

const std::unordered_map<std::string, std::string> MIME_MAP {
    { HTTP::EXT::HTML , HTTP::MIME::TEXT_HTML },
    { HTTP::EXT::CSS  , HTTP::MIME::TEXT_CSS },
    { HTTP::EXT::JS   , HTTP::MIME::APPLICATION_JAVASCRIPT },
    { HTTP::EXT::JPG  , HTTP::MIME::IMAGE_JPEG },
    { HTTP::EXT::JPEG , HTTP::MIME::IMAGE_JPEG },
    { HTTP::EXT::PNG  , HTTP::MIME::IMAGE_PNG },
    { HTTP::EXT::GIF  , HTTP::MIME::IMAGE_GIF },
    { HTTP::EXT::SWF  , HTTP::MIME::APPLICATION_X_SHOCKWAVE_FLASH },
};

class server {
public:
    static std::string _document_root;

    server() = delete;

    server(
        const std::string& address,
        std::uint16_t port, 
        std::size_t num_of_threads,
        const std::string& doc_root
    );

    ~server() = default;

    int run();

    static void handle_request(evhttp_request* req);

    static void add_default_headers(evkeyvalq* headers);

    static std::pair<int,std::size_t> find_source(const std::string& root, const std::string& path);

    static std::string get_date();

    static std::string get_content_type(const std::string& path);

private:
    std::string _address;
    std::uint16_t _port;
    std::size_t _number_of_threads;
    thread_pool _thread_pool;

    void (*on_request)(evhttp_request* req, void* tp) = [] (evhttp_request *req, void* tp) {
        BOOST_LOG_TRIVIAL(info) << "New client has come with uri: " << req->uri;
        static_cast<thread_pool*>(tp)->enqueue(handle_request, req);
    };
};

#endif // SERVER_HPP