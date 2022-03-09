#ifndef HTTP_REQUEST
# define HTTP_REQUEST

# include <string>
# include <map>

struct http_request
{
    http_request();
    http_request(bool r);
    ~http_request();

    static http_request reject_http_request(void);

    /* Request Line RFC7230/3.1.1. */
    std::string                                     method_token;
    std::string                                     target; /* abs_path RFC2396/3., ex. /about.txt?q=hi */
    std::string                                     protocol_version;

    std::map<std::string, std::string>              header_fields;
    std::string                                     payload; /* message body RFC7230/3.3.*/
    bool                                            reject;

    /* Information of the sender of the request */
    std::string                                     hostname; /* ipv4 | ipv6 */
    std::string                                     port;

    /* Absolute Uniform Resource Identifier and its components RFC2396/3.
    * opaque_part is not implemented
    */
    std::string                                     URI;
    std::string                                     scheme;
    std::string                                     net_path;
    std::string                                     abs_path;
    std::string                                     query;

    int                                             socket; /* request is read from this socket */
};

#endif
