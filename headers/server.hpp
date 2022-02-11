#ifndef SERVER_HPP
# define SERVER_HPP

# include "header.hpp"
# include <set>
# include <map>
# include <vector>

class server
{
private:
    int                                 server_socket_fd;
    int                                 server_port;
    int                                 server_backlog;
    fd_set                              connected_sockets;
    std::set<int>                       connected_sockets_set;
    std::map<std::string, std::string>  cachedFiles; /* route - content */
    std::set<std::string>               acceptedMethods;
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
    int     read_client_message(int socket);
    int     parse_message(const std::map<std::string, std::string>& message);
    void    router(int socket, int request);

    enum requestCodes
    {
        INDEX,
        ABOUT,
        ERROR
    };
};

#endif
