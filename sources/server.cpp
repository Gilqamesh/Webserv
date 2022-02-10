#include "server.hpp"

server::server(int port, int backlog)
    : server_socket_fd(-1), server_port(port), server_backlog(backlog)
{
    /* creating a socket (domain/address family, type of service, specific protocol)
    * AF_INET       -   IP address family
    * SOCK_STREAM   -   virtual circuit service
    * 0             -   only 1 protocol exists for this service
    */
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1)
        TERMINATE("failed to create a socket");

    /* binding/naming the socket
    * 1. fill out struct sockaddr_in
    *   sin_family    -   address family used for the socket
    *   sin_addr      -   address for the socket, it's the machine's IP address
    *                 -   INADDR_ANY is a special address 0.0.0.0
    *   sin_port      -   port number, if you are client set it to 0 to let the OS decide
    * 2. bind the socket (socket, socket address, address length)
    */
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(server_port);
    if (bind(server_socket_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == -1)
        TERMINATE("failed to bind the socket");

    /* wait/listen for connection
    * listen(socket, backlog)
    *   the backlog defines the maximum number of pending connections that can be queued up
    *   before connections are refused
    */
    if (listen(server_socket_fd, server_backlog) == -1)
        TERMINATE("listen failed");
    LOG("Server listens on port: " << server_port);
    connected_sockets_set.insert(server_socket_fd);
    FD_ZERO(&connected_sockets);
    FD_SET(server_socket_fd, &connected_sockets);

    /* ready_sockets
    * will be a copy of connected_sockets each iteration,
    * and is needed because select is destructive
    */
    while (true)
    {
        fd_set ready_sockets = connected_sockets;
        struct timeval timeout = { TIMEOUT_TO_CUT_CONNECTION, 0 };
        /* select()
        * monitors all file descriptors and waits until one of them is ready
        */
        if (select(*(--connected_sockets_set.end()) + 1, &ready_sockets, NULL, NULL, &timeout) == -1)
            TERMINATE("select failed");
        for (std::set<int>::const_iterator cit = connected_sockets_set.begin(); cit != connected_sockets_set.end(); ++cit)
        {
            if (FD_ISSET(*cit, &ready_sockets))
            {
                if (*cit == server_socket_fd) { /* new connection */
                    accept_connection();
                } else { /* connection is ready to write/read */
                    handle_connection(*cit);
                }
            }
        }
    }
}

server::server(const server &other)
    : server_socket_fd(other.server_socket_fd),
    server_port(other.server_port),
    server_backlog(other.server_backlog),
    connected_sockets(other.connected_sockets),
    connected_sockets_set(other.connected_sockets_set)
{

}

server &server::operator=(const server &other)
{
    if (this != &other)
    {
        server_socket_fd = other.server_socket_fd;
        server_port = other.server_port;
        server_backlog = other.server_backlog;
        connected_sockets = other.connected_sockets;
        connected_sockets_set = other.connected_sockets_set;
    }
    return (*this);
}

server::~server()
{
    close(server_socket_fd);
}

void server::accept_connection(void)
{
    /* do not accept connection if the backlog is full */
    if (connected_sockets_set.size() >= static_cast<unsigned long>(server_backlog))
        return ;
    /* accept(socket, address, socket length) - grabs the first connection request on the queue
    * of pending connections and creates a new socket for that connection
    * The original socket that is set up for listening is used only for accepting connections,
    * not for exchanging data.
    */
    struct sockaddr_in address;
    socklen_t address_length;
    int new_socket = accept(server_socket_fd, reinterpret_cast<struct sockaddr *>(&address), &address_length);
    if (new_socket == -1)
        TERMINATE("accept failed");
    FD_SET(new_socket, &connected_sockets);
    connected_sockets_set.insert(new_socket);
    LOG("Client joined from port: " << ntohs(address.sin_port));
}

void server::cut_connection(int socket)
{
    /* close socket after we are done communicating */
    close(socket);
    FD_CLR(socket, &connected_sockets);
    connected_sockets_set.erase(socket);
    LOG("Client disconnected on socket: " << socket);
}

void server::handle_connection(int socket)
{
    std::string msg("hello");
    write(socket, msg.c_str(), msg.length());
}
