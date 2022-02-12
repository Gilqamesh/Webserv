#ifndef SERVER_HPP
# define SERVER_HPP

# include "header.hpp"
# include <set>
# include <map>
# include <unordered_map>
# include <vector>

# define HEADER_WHITESPACES " \t"

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

    void    accept_connection(void);
    void    cut_connection(int socket);
    void    handle_connection(int socket);
    int     parse_request_header(int socket);
    int     parse_message(const std::unordered_map<std::string, std::string>& message);
    void    router(int socket, int status_code, int request);

    void    initialize_constants(void); // helper

    enum requestCodes
    {
        INDEX,
        ABOUT,
        ERROR
    };
};

#endif
