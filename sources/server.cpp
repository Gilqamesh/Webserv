#include "server.hpp"
#include "HandleHTTPRequest.hpp"
#include "Utils.hpp"
#include <cstdio>

void server::initialize_constants(void)
{
    /* add allowed request methods for clients */
    accepted_request_methods.insert("GET");
    accepted_request_methods.insert("POST");
    accepted_request_methods.insert("DELETE");
    /* whitespaces in header fields rfc7230/3.2.3 */
    header_whitespace_characters.insert(' ');
    header_whitespace_characters.insert('\t');
}

server::server(int port, int backlog)
    : server_socket_fd(-1), server_port(port), server_backlog(backlog)
{
    // PRINT_HERE();
    initialize_constants();
    // PRINT_HERE();

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
    accepted_request_methods(other.accepted_request_methods)
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
        accepted_request_methods = other.accepted_request_methods;
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
    int status_code = parse_request_header(socket);
    router(socket, status_code, INDEX);
    cut_connection(socket);
}

/* Message format (rfc7230/3.)
* 1. start-line (request-line for request, status-line for response)
* 2. *( header-field CRLF)
* 3. CRLF
* 4. optional message-body (not implemented)
* Return value: http status code
*/
int server::parse_request_header(int socket)
{
    std::unordered_map<std::string, std::string> message;
    std::string current_line = get_next_line(socket);
    /* Read and store request line
    * syntax checking for the request line happens here (rfc7230/3.1.1.) - this part could be
    *   done better with regex
    */
    std::string method = current_line.substr(0, current_line.find_first_of(' '));
    std::string tmp = current_line.substr(current_line.find_first_of(' ') + 1);
    std::string request_target = tmp.substr(0, tmp.find_first_of(' '));
    std::string::size_type index = tmp.find_first_of(' ');
    if (index == std::string::npos) /* 400 bad request (syntax error) */
        return (ERROR);
    std::string protocol_version = tmp.substr(index + 1);
    if (accepted_request_methods.count(method) == 0) /* 400 bad request (syntax error) */
        return (ERROR);
    if (cachedFiles.count(request_target) == 0) /* 404 not found */
        return (ERROR);
    assert(CRLF == "\x0d"); /* CR without LF for some reason, maybe get_next_line above is messed up */
    if (protocol_version != "HTTP/1.1" + CRLF) /* 426 upgrade on protocol is required */
        return (ERROR);
    message["request-line"] = current_line.substr(0, current_line.find_first_of(CRLF));
    LOG(message["request-line"]);

    bool first_header_field = true;
    /* Read and store header fields
    * store each key value pair in 'message'
    * appending field-names that are of the same name happens here (RFC7230/3.2.2.)
    * syntax checking for the header fields happens here
    * if wrong syntax: server must reject the message, proxy should remove them
    * bad (BWS) and optional whitespaces (OWS) are getting removed here
    */
    /* Header field format (rfc7230/3.2.)
    * field-name ":" OWS field-value OWS
    */
    while ((current_line = get_next_line(socket)).size())
    {
        if (first_header_field == true) { /* RFC7230/3. A sender MUST NOT send whietspace between the start-line and the first header field */
            if (header_whitespace_characters.count(current_line[0]))
                return (ERROR); /* 400 bad request (syntax error) */
            first_header_field = false;
        }
        if (current_line == CRLF)
            break ;
        std::string::iterator it = current_line.begin();
        while (it != current_line.end() && *it != ':')
        {
            if (header_whitespace_characters.count(*it))
                return (ERROR); /* 400 bad request (syntax error) */
            ++it;
        }
        if (it == current_line.end())
            return (ERROR); /* 400 bad request (syntax error) */
        std::string field_name(current_line.begin(), it);
        ++it;
        while (it != current_line.end() && header_whitespace_characters.count(*it)) /* remove preceding whitespaces */
            ++it;
        if (it == current_line.end())
            return (ERROR); /* 400 bad request (syntax error) */
        std::string field_value(it, current_line.end());
        std::string field_value_clean(field_value.substr(field_value.find_first_not_of(HEADER_WHITESPACES), field_value.find_first_of(HEADER_WHITESPACES)));
        message[field_name] = field_value_clean.substr(0, field_value.find_last_of(CRLF));
    }
    return (parse_message(message));
}

/*
* parses the message and return a request
* don't know what makes most sense to handle header communication yet..
*/
int server::parse_message(const std::unordered_map<std::string, std::string>& message)
{
    HandleHTTPRequest client_http_request(message);
    return (client_http_request.get_request_code());
}

/*
* serves request on socket
*/
void server::router(int socket, int status_code, int request)
{
    /* construct response message
    *
    * first line is the Status Line (rfc7230/3.1.2.)
    */
    std::string message = "HTTP/1.1 " + std::to_string(status_code) + "\nContent-Type:text/html\nContent-Length: ";
    if (request == INDEX)
        message += std::to_string(cachedFiles["/"].length()) + "\n\n" + cachedFiles["/"];
    else if (request == ABOUT)
        message += std::to_string(cachedFiles["/about"].length()) + "\n\n" + cachedFiles["/about"];
    else
        message += std::to_string(cachedFiles["/error"].length()) + "\n\n" + cachedFiles["/error"];
    write(socket, message.c_str(), message.length());
}
