#ifndef HTTP_REQUEST
# define HTTP_REQUEST

# include <string>
# include <unordered_map>

struct http_request
{
    http_request();
    http_request(bool r);
    http_request(const http_request &other);
    http_request &operator=(const http_request &other);
    ~http_request();

    static http_request reject_http_request(void);

    std::string                                     method_token;
    std::string                                     target;
    std::string                                     protocol_version;
    std::unordered_map<std::string, std::string>    header_fields;
    std::string                                     payload; /* message body RFC7230/3.3.*/
    bool                                            reject;
};

#endif
