#ifndef HTTPMESSAGE
# define HTTPMESSAGE

# include <string>
# include <unordered_map>

struct http_message
{
    http_message();
    http_message(bool r);
    http_message(const http_message &other);
    http_message &operator=(const http_message &other);
    ~http_message();

    static http_message reject_http_message(void);

    std::string                                     method_token;
    std::string                                     target;
    std::string                                     protocol_version;
    std::unordered_map<std::string, std::string>    header_fields;
    bool                                            reject;
};

#endif
