#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <string>
#include <map>
#include <sstream>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code;
    std::string status_text;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse(int code = 200, const std::string& text = "OK") 
        : status_code(code), status_text(text) {}
};

HttpRequest parse_http_request(const std::string& raw_request);
std::string build_http_response(const HttpResponse& response);
std::string get_mime_type(const std::string& path);
std::string url_decode(const std::string& str);

#endif
