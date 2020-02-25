#include <string>
#include <chrono>
#include <event2/http.h>
#include <utility>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>

#include "server.hpp"

namespace fs = boost::filesystem;

std::string server::_document_root = "/var/www/html";

server::server(const std::string& address, std::uint16_t port, 
    std::size_t num_of_threads, const std::string& doc_root)
    : _address(address)
    , _port(port)
    , _number_of_threads(num_of_threads)
    , _thread_pool()
    {
        _document_root = doc_root;
    };

int server::run() {
    _thread_pool.start(_number_of_threads);

    if (!event_init()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to init libevent.";
        return EXIT_FAILURE;
    }

    std::unique_ptr<evhttp, decltype(&evhttp_free)> 
        Server(evhttp_start(_address.c_str(), _port), &evhttp_free);

    if (!Server)  {
        BOOST_LOG_TRIVIAL(error) << "Failed to init http server.";
        return EXIT_FAILURE;
    }

    evhttp_set_gencb(Server.get(), on_request, &_thread_pool);

    if (event_dispatch() == -EXIT_FAILURE) {
        BOOST_LOG_TRIVIAL(error) << "Failed to run message loop.";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

 // Date: <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
std::string server::get_date() {
    std::time_t timer = std::time(nullptr);
    char date_buffer[MAX_DATE_BUFFER_SIZE];
    auto now = std::strftime(date_buffer, sizeof(date_buffer), 
        HTTP::DATE_FORMAT, std::localtime(&timer));

    return std::string(date_buffer, now);
}

void server::add_default_headers(evkeyvalq* headers){
    if (headers == nullptr)
        return;
    evhttp_add_header(headers, HTTP::HEADER::SERVER, "libevent (UNIX)");

    auto date = get_date();
    evhttp_add_header(headers, HTTP::HEADER::DATE, date.c_str());

    evhttp_add_header(headers, HTTP::HEADER::CONNECTION, "close");
};

std::pair<int,std::size_t> server::find_source(const std::string& root, const std::string& path) {
    fs::path fs_path = fs::path(root) / fs::path(path);
    if (path[path.length() - 1] == '/') {
        fs_path /= fs::path("index.html");
    }
    BOOST_LOG_TRIVIAL(debug) << "Full path to file: " << fs_path.c_str();

    int file_fd = open(fs_path.c_str(), O_RDONLY);
    BOOST_LOG_TRIVIAL(debug) << "file_fd: " << file_fd;
    if (file_fd == -1) {
        return std::make_pair(file_fd, 0);
    }
    
    BOOST_LOG_TRIVIAL(debug) << "file_size: " << fs::file_size(fs_path);
    return std::make_pair(file_fd, fs::file_size(fs_path));
}

std::string server::get_content_type(const std::string& path) {
    auto origin_path = path;
    if (path[path.length()-1] == '/') {
        origin_path += "index.html";
    }
    fs::path fs_path(path); 
    const std::string ext = fs_path.filename().extension().c_str();
    if (ext == HTTP::EXT::HTML) {
        return HTTP::MIME::TEXT_HTML;
    } else if (ext == HTTP::EXT::CSS) {
        return HTTP::MIME::TEXT_CSS;
    } else if (ext == HTTP::EXT::JS) {
        return HTTP::MIME::APPLICATION_JAVASCRIPT;
    } else if (ext == HTTP::EXT::JPG) {
        return HTTP::MIME::IMAGE_JPEG;
    } else if (ext == HTTP::EXT::JPEG) {
        return HTTP::MIME::IMAGE_JPEG;
    } else if (ext == HTTP::EXT::PNG) {
        return HTTP::MIME::IMAGE_PNG;
    } else if (ext == HTTP::EXT::GIF) {
        return HTTP::MIME::IMAGE_GIF;
    } else if (ext == HTTP::EXT::SWF) {
        return HTTP::MIME::APPLICATION_X_SHOCKWAVE_FLASH;
    } else {
        return HTTP::MIME::APPLICATION_OCTET_STREAM;
    }
}

bool server::is_safe_path(const std::string& path) {
    auto double_dot = path.find_first_of("..");
    if (double_dot == std::string::npos)
        return true;
    auto last_slash = path.find_last_of('/');
    if (last_slash < double_dot)
        return true;
    return false;
}

void server::handle_request(evhttp_request *req) {
    bool is_index = false;
    auto out_headers = evhttp_request_get_output_headers(req);
    add_default_headers(out_headers);

    // only GET and HEAD requests allowed
    if (req->type != EVHTTP_REQ_GET && req->type != EVHTTP_REQ_HEAD) {
        BOOST_LOG_TRIVIAL(info) << HTTP_BADMETHOD << ' ' << req->uri << std::endl;
        evhttp_send_reply(req, HTTP_BADMETHOD, nullptr, nullptr);    
        return;
    }
    // decoded URI
    auto decoded_uri = evhttp_uri_parse(req->uri);
	if (!decoded_uri) {
		BOOST_LOG_TRIVIAL(error) << "Bad URI: " <<  req->uri;
        BOOST_LOG_TRIVIAL(info) << HTTP_BADREQUEST << ' ' << req->uri << std::endl;
		evhttp_send_reply(req, HTTP_BADMETHOD, nullptr, nullptr);    
		return;
	}
    // path from URI
    auto path = evhttp_uri_get_path(decoded_uri);
	if (!path) {
        path = "/";
        is_index = true;
    };
    // decoded path
    auto decoded_path = evhttp_uridecode(path, 0, nullptr);
	if (!decoded_path) {
        BOOST_LOG_TRIVIAL(error) << "Bad path:" << path;
        BOOST_LOG_TRIVIAL(info) << HTTP_NOTFOUND << ' ' << req->uri << std::endl;
        evhttp_send_reply(req, HTTP_NOTFOUND, nullptr, nullptr);   
        evhttp_uri_free(decoded_uri);
        return;
    } else {
        is_index = (decoded_path[strlen(decoded_path)-1] == '/');
    }

    if (!is_safe_path(decoded_path)) {
        BOOST_LOG_TRIVIAL(error) << "Bad decoded path: " << decoded_path;
        BOOST_LOG_TRIVIAL(info) << HTTP_NOTFOUND << ' ' << req->uri << std::endl;
        evhttp_send_reply(req, HTTP_NOTFOUND, nullptr, nullptr);
        evhttp_uri_free(decoded_uri);
        free(decoded_path);
        return;
    }

    auto result = server::find_source(_document_root, decoded_path);

    auto out = evbuffer_new();

    if (result.first == -1) {
        BOOST_LOG_TRIVIAL(debug) << "File not found: " << decoded_path;
        if (is_index) {
            BOOST_LOG_TRIVIAL(info) << HTTP::FORBIDDEN << ' ' << req->uri << std::endl;
            evhttp_send_reply(req, HTTP::FORBIDDEN, nullptr, nullptr);
        } else {
            BOOST_LOG_TRIVIAL(info) << HTTP_NOTFOUND << ' ' << req->uri << std::endl;
	        evhttp_send_reply(req, HTTP_NOTFOUND, nullptr, nullptr);
        }
    } else {
        if (req->type == EVHTTP_REQ_GET) {
            evbuffer_add_file(out, result.first, 0, result.second);
        }
        // Add headers
        auto content_type = get_content_type(decoded_path);
        BOOST_LOG_TRIVIAL(debug) << "Content-type: " <<  content_type;
        evhttp_add_header(out_headers, HTTP::HEADER::CONTENT_TYPE, content_type.c_str());

        auto content_length = std::to_string(result.second);
        BOOST_LOG_TRIVIAL(debug) << "Content-length: " << content_length;
        evhttp_add_header(out_headers, HTTP::HEADER::CONTENT_LENGTH, content_length.c_str());

        BOOST_LOG_TRIVIAL(info) << HTTP_OK << ' ' << req->uri << std::endl;
        evhttp_send_reply(req, HTTP_OK, "OK", out);
    }
    
	if (decoded_uri)
		evhttp_uri_free(decoded_uri);
    if (decoded_path)
		free(decoded_path);
    if (out)
		evbuffer_free(out);
}