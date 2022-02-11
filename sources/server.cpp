#include "server.hpp"
#include "HandleHTTPRequest.hpp"
#include "Utils.hpp"
#include <cstdio>

server::server(int port, int backlog)
    : server_socket_fd(-1), server_port(port), server_backlog(backlog)
{
    /*
    * add allowed methods for clients
    */
    acceptedMethods.insert("GET");
    acceptedMethods.insert("POST");
    acceptedMethods.insert("DELETE");

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
}

server::server(const server &other)
    : server_socket_fd(other.server_socket_fd),
    server_port(other.server_port),
    server_backlog(other.server_backlog),
    connected_sockets(other.connected_sockets),
    connected_sockets_set(other.connected_sockets_set),
    cachedFiles(other.cachedFiles),
    acceptedMethods(other.acceptedMethods)
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
        cachedFiles = other.cachedFiles;
        acceptedMethods = other.acceptedMethods;
    }
    return (*this);
}

server::~server()
{
    close(server_socket_fd);
}

void server::server_listen(void)
{
    while (true)
    {
        /* ready_sockets
        * a copy of connected_sockets each iteration,
        * is needed because 'select' is destructive
        */
        fd_set ready_sockets = connected_sockets;
        struct timeval timeout = { TIMEOUT_TO_CUT_CONNECTION, 0 };
        /* select()
        * monitors all file descriptors and waits until one of them is ready
        * first argument is the highest possible socket number + 1
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

void server::cache_file(const std::string &path, const std::string &route)
{
    if (cachedFiles.count(route))
    {
        WARN("route: '" << route << "' already exists");
        return ;
    }
    std::ifstream ifs(path, std::ifstream::in);
    if (!ifs)
    {
        WARN("file '" + path + "' could not be opened");
        return ;
    }
    std::string tmp;
    std::string content;
    while (getline(ifs, tmp))
        content += tmp;
    cachedFiles[route] = content;
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
    LOG("Client joined from socket: " << new_socket);
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
    int request = read_client_message(socket);
    router(socket, request);
    cut_connection(socket);
}

/*
* returns with the request
* for now read until EOF, but this is not good practice
* because client can make the server hang
*/
int server::read_client_message(int socket)
{
    std::map<std::string, std::string> message;
    std::string tmp = get_next_line(socket);
    /* check for the validity of request line
    * should being with a method token
    * followed by a single space then the request-target
    * followed by a single space then the protocol version
    * ends with a CRLF
    */
    std::string method = tmp.substr(0, tmp.find_first_of(' '));
    LOG("method: '" << method << "'");
    std::string tmp2 = tmp.substr(tmp.find_first_of(' ') + 1);
    std::string request_target = tmp2.substr(0, tmp2.find_first_of(' '));
    LOG("request_target: '" << request_target << "'");
    std::string::size_type index = tmp2.find_first_of(' ');
    if (index == std::string::npos)
    {
        PRINT_HERE();
        return (ERROR);
    }
    std::string protocol_version = tmp2.substr(index + 1);
    LOG("index: " << index);
    LOG("protocol version: " << protocol_version);

    LOG(tmp);

    if (acceptedMethods.count(method) == 0)
    {
        PRINT_HERE();
        return (ERROR); /* 400 bad request (syntax error) */
    }
    if (cachedFiles.count(request_target) == 0) /* 404 not found */
    {
        PRINT_HERE();
        return (ERROR);
    }
    TERMINATE(protocol_version.c_str());
    if (protocol_version != std::string("HTTP/1.1\n")) /* 426 upgrade on protocol is required */
    {
        PRINT_HERE();
        return (ERROR);
    }

    while ((tmp = get_next_line(socket)).size())
    {
        LOG(tmp);
        std::string::size_type index = tmp.find_first_of(':');
        if (index == std::string::npos)
            return (ERROR); /* 400 bad request (syntax error) */
        std::string key = tmp.substr(0, index);
        message[key] = tmp.substr(index + 1);
    }
    PRINT_HERE();
    for (std::map<std::string, std::string>::const_iterator cit = message.begin(); cit != message.end(); ++cit)
        LOG(cit->first << " " << cit->second);
    PRINT_HERE();
    return (parse_message(message));
}

/*
* parses the message and return a request
* don't know what makes most sense to handle header communication yet..
*/
int server::parse_message(const std::map<std::string, std::string>& message)
{
    HandleHTTPRequest client_http_request(message);
    return (client_http_request.get_request_code());
}

/*
* serves request on socket
*/
void server::router(int socket, int request)
{
    std::string message;
    if (request == INDEX)
        message = "HTTP/1.1 200 OK\nContent-Type:text/html\nContent-Length: "
        + std::to_string(cachedFiles["/"].length()) + "\n\n" + cachedFiles["/"];
    else if (request == ABOUT)
        message = "HTTP/1.1 200 OK\nContent-Type:text/html\nContent-Length: "
        + std::to_string(cachedFiles["/about"].length()) + "\n\n" + cachedFiles["/about"];
    else
        message = "HTTP/1.1 404 OK\nContent-Type:text/html\nContent-Length: "
        + std::to_string(cachedFiles["/error"].length()) + "\n\n" + cachedFiles["/error"];
    write(socket, message.c_str(), message.length());
}
