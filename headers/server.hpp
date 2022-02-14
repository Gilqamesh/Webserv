#ifndef SERVER_HPP
# define SERVER_HPP

# include "header.hpp"
# include <set>
# include <unordered_set>
# include <map>
# include <unordered_map>
# include <vector>
# include "http_request.hpp"

# define HEADER_WHITESPACES " \t"
# define HEADER_FIELD_PATTERN "[^ \t:]*:[ \t]*[ -~]*[ \t]*" + CRLF
# define HEADER_REQUEST_LINE_PATTERN "[!-~]+ [!-~]+ HTTP/[1-3][.]*[0-9]*" + CRLF /* match_pattern needs support on '?' and '()' (capture group) for proper parsing */
# define HEADER_RESPONSE_LINE_PATTERN "[!-~]+ [!-~]+ [!-~]*" + CRLF

class server
{
private:
    int                                             server_socket_fd;
    int                                             server_port;
    int                                             server_backlog;
    fd_set                                          connected_sockets;
    std::map<int, unsigned long>                    connected_sockets_map; /* socket - timestamp */
    std::unordered_map<std::string, std::string>    cachedFiles; /* route - content */
    /* constants */
    std::unordered_set<std::string>                 accepted_request_methods;
    std::unordered_set<char>                        header_whitespace_characters;
    unsigned long                                   start_timestamp;
public:
    server(int port, int backlog);
    ~server();

    void server_listen(void);
    void cache_file(const std::string &path, const std::string &route);
private:
    server();
    server(const server& other);
    server &operator=(const server& other);

    void            accept_connection(void);
    void            cut_connection(int socket);
    void            handle_connection(int socket);
    http_request    parse_request_header(int socket);
    void            router(int socket, const http_request& request);

    /* format http request and its control functions */
    void            format_http_request(http_request& request);
    void            handle_cache_control(void);
    void            handle_expect(void);
    void            handle_host(void);
    void            handle_max_forwards(void);
    void            handle_pragma(void);
    void            handle_range(void);
    void            handle_TE(void);

    void            initialize_constants(void); // helper
};

#endif
