#include <string>
#include <chrono>
#include <evhttp.h>
#include <utility>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>

#include "server.hpp"

namespace fs = boost::filesystem;

std::string server::_document_root = "";

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

    int file_fd = open(fs_path.string().c_str(), O_RDONLY);
    return std::make_pair(file_fd, fs::file_size(fs_path));
}

std::string server::get_content_type(const std::string& path) {
    auto origin_path = path;
    if (path[path.length()-1] == '/') {
        origin_path += "index.html";
    }
    fs::path fs_path(path); 
    const std::string ext = fs_path.filename().extension().c_str();
    auto it = MIME_MAP.find(ext);
    if (it == MIME_MAP.end())
        return HTTP::MIME::APPLICATION_OCTET_STREAM;
    return it->second;
}

void server::handle_request(evhttp_request *req) {
    auto out_headers = evhttp_request_get_output_headers(req);
    add_default_headers(out_headers);

    // only GET and HEAD requests allowed
    if (req->type != EVHTTP_REQ_GET && req->type != EVHTTP_REQ_HEAD) {
        evhttp_send_error(req, HTTP_BADMETHOD, nullptr);
        BOOST_LOG_TRIVIAL(debug) << "Job have done by thread: " << std::this_thread::get_id();
        return;
    }
    // decoded uri
    auto decoded_uri = evhttp_uri_parse(req->uri);
	if (!decoded_uri) {
		BOOST_LOG_TRIVIAL(error) << "It's not a good URI. Sending BADREQUEST" ;
		evhttp_send_error(req, HTTP_BADREQUEST, nullptr);
		return;
	}
    
    auto path = evhttp_uri_get_path(decoded_uri);
	if (!path) path = "/";

    auto decoded_path = evhttp_uridecode(path, 0, nullptr);
	if (!decoded_path) {
        BOOST_LOG_TRIVIAL(error) << "It's not a good path. Sending BADREQUEST" ;
        evhttp_send_error(req, HTTP_NOTFOUND, nullptr);
        evhttp_uri_free(decoded_uri);
        return;
    }
	/* Don't allow any ".."s in the path, to avoid exposing stuff outside
	 * of the docroot.  This test is both overzealous and underzealous:
	 * it forbids aceptable paths like "/this/one..here", but it doesn't
	 * do anything to prevent symlink following." */
	if (strstr(decoded_path, "..")) {
        BOOST_LOG_TRIVIAL(error) << "It's not a good path. Sending BADREQUEST" ;
        evhttp_send_error(req, HTTP_NOTFOUND, nullptr);
        evhttp_uri_free(decoded_uri);
        free(decoded_path);
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "path: " << decoded_path;

    // logic with files...
    auto result = server::find_source(_document_root, decoded_path);

    auto out = evbuffer_new();
    if (result.first == -1) {
	    evhttp_send_error(req, HTTP_NOTFOUND, nullptr);
    } else {
        if (req->type == EVHTTP_REQ_GET) {
            evbuffer_add_file(out, result.first, 0, result.second);
        }
        // Add headers
        evhttp_add_header(out_headers, HTTP::HEADER::CONTENT_TYPE, get_content_type(decoded_path).c_str());
        std::string l;
        std::stringstream ss;
        ss << result.second;
        evhttp_add_header(out_headers, HTTP::HEADER::CONTENT_LENGTH, ss.str().c_str());

        evhttp_send_reply(req, HTTP_OK, "OK", out);     
    }
    
	if (decoded_uri)
		evhttp_uri_free(decoded_uri);
    if (decoded_path)
		free(decoded_path);
    if (out)
		evbuffer_free(out);

    BOOST_LOG_TRIVIAL(info) << "Job have done by thread: "
        << std::this_thread::get_id() << std::endl;
}