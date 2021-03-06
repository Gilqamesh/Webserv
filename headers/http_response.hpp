#ifndef HTTP_RESPONSE
# define HTTP_RESPONSE

# include <string>
# include <map>

struct http_response
{
    http_response();
    ~http_response();

    static http_response reject_http_response();
    static http_response cgi_response();

    static http_response tooLargeResponse();

    /* Status Line RFC7230/3.1.2. */
    std::string                                     http_version;
    std::string                                     status_code;
    std::string                                     reason_phrase;

    std::map<std::string, std::string>              header_fields;
    std::string                                     payload; /* message body RFC7230/3.3.*/

    bool                                            reject;
    bool                                            handled_by_cgi;
};

#endif
