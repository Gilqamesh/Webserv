#ifndef SERVER_HPP
# define SERVER_HPP

# include "header.hpp"
# include <set>
# include <map>
# include <unordered_map>
# include <vector>
# include "http_message.hpp"

# define HEADER_WHITESPACES " \t"
# define HEADER_FIELD_PATTERN "[^ \t:]*:[ \t]*[ -~]*[ \t]*" + CRLF
# define HEADER_REQUEST_LINE_PATTERN "[^ ]* [^ ]* [^ ]*" + CRLF

class server
{
private:
    int                                             server_socket_fd;
    int                                             server_port;
    int                                             server_backlog;
    fd_set                                          connected_sockets;
    std::set<int>                                   connected_sockets_set;
    std::unordered_map<std::string, std::string>    cachedFiles; /* route - content */
    /* constants */
    std::set<std::string>                           accepted_request_methods;
    std::set<char>                                  header_whitespace_characters;
public:
    server(int port, int backlog);
    server(const server& other);
    server &operator=(const server& other);
    ~server();

    void server_listen(void);
    void cache_file(const std::string &path, const std::string &route);
private:
    server();

    void            accept_connection(void);
    void            cut_connection(int socket);
    void            handle_connection(int socket);
    http_message    parse_request_header(int socket);
    http_message    parse_message(const http_message& message);
    void            router(int socket, const http_message& request);

    void            initialize_constants(void); // helper

    enum requestCodes
    {
        INDEX,
        ABOUT,
        ERROR
    };
};

#endif
