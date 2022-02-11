#ifndef HANDLEHTTPREQUEST_HPP
# define HANDLEHTTPREQUEST_HPP

# include <string>
# include <map>

class HandleHTTPRequest
{
public:
    HandleHTTPRequest();
    HandleHTTPRequest(const std::string &request);
    HandleHTTPRequest(const HandleHTTPRequest& other);
    HandleHTTPRequest &operator=(const HandleHTTPRequest& other);
    ~HandleHTTPRequest();

    int get_request_code(void);

private:
    std::string                         request_message;
    std::map<std::string, std::string>  cache_control;
    int                                 request_code;

    void handle_cache_control(void);
    void handle_expect(void);
    void handle_host(void);
    void handle_max_forwards(void);
    void handle_pragma(void);
    void handle_range(void);
    void handle_TE(void);
};

# endif
