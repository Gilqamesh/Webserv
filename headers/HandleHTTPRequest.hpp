#ifndef HANDLEHTTPREQUEST_HPP
# define HANDLEHTTPREQUEST_HPP

# include <string>
# include <unordered_map>
# include "http_message.hpp"

class HandleHTTPRequest
{
public:
    HandleHTTPRequest();
    HandleHTTPRequest(const http_message &message);
    HandleHTTPRequest(const HandleHTTPRequest& other);
    HandleHTTPRequest &operator=(const HandleHTTPRequest& other);
    ~HandleHTTPRequest();

    int get_request_code(void);

private:
    http_message    request_message;
    int             request_code;

    void handle_cache_control(void);
    void handle_expect(void);
    void handle_host(void);
    void handle_max_forwards(void);
    void handle_pragma(void);
    void handle_range(void);
    void handle_TE(void);
};

# endif
