#ifndef SERVER_HPP
# define SERVER_HPP

# include "header.hpp"
# include <set>

class server
{
private:
    int             server_socket_fd;
    int             server_port;
    int             server_backlog;
    fd_set          connected_sockets;
    std::set<int>   connected_sockets_set;
public:
    server(int port, int backlog);
    server(const server& other);
    server &operator=(const server& other);
    ~server();

private:
    server();

    void    accept_connection(void);
    void    cut_connection(int socket);
    void    handle_connection(int socket);
};

#endif
