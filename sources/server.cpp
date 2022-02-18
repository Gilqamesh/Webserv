#include "server.hpp"
#include "utils.hpp"
#include <cstdio>

/*
* helper to initialize some constants
*/
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
    initialize_constants();

    /* creating a socket (domain/address family, type of service, specific protocol)
    * AF_INET       -   IP address family
    * SOCK_STREAM   -   virtual circuit service
    * 0             -   only 1 protocol exists for this service
    */
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1)
        TERMINATE("failed to create a socket");

    /* set socket opt
    * SO_REUSEADDR allows us to reuse the specific port even if that port is busy
    */
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &server_socket_fd, sizeof(int)) == -1)
        TERMINATE("setsockopt failed");

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
    connected_sockets_map.insert(std::make_pair<int, unsigned long>(server_socket_fd, get_current_timestamp()));
    FD_ZERO(&connected_sockets);
    FD_SET(server_socket_fd, &connected_sockets);
}

server::~server()
{
    while (connected_sockets_map.size())
        cut_connection(connected_sockets_map.begin()->first);
}

/*
* main loop of the server
* waits for new connections or handles already established connections
*/
void server::server_listen(void)
{
    start_timestamp = get_current_timestamp();
    struct timeval timeout = { TIMEOUT_TO_CUT_CONNECTION, 0 };
    while (true)
    {
        /* ready_sockets
        * a copy of connected_sockets each iteration,
        * is needed because 'select' is destructive
        */
        fd_set ready_sockets = connected_sockets;
        /* select()
        * monitors all file descriptors and waits until one of them is ready
        * first argument is the highest possible socket number + 1
        */
        if (select((--connected_sockets_map.end())->first + 1, &ready_sockets, NULL, NULL, &timeout) == -1)
            TERMINATE("select failed");
        unsigned long current_timestamp = get_current_timestamp();
        unsigned long minimum_timestamp = current_timestamp; /* to update 'timeout' */
        for (std::map<int, unsigned long>::iterator it = connected_sockets_map.begin(); it != connected_sockets_map.end();)
        {
            std::map<int, unsigned long>::iterator tmp = it++; /* have to do this because iterator can get invalidated */
            if (FD_ISSET(tmp->first, &ready_sockets))
            {
                if (tmp->first == server_socket_fd) { /* new connection */
                    accept_connection();
                } else { /* connection is ready to write/read */
                    tmp->second = get_current_timestamp(); /* update timestamp */
                    handle_connection(tmp->first);
                }
            }
            else if (tmp->first != server_socket_fd)
            {
                if (tmp->second + TIMEOUT_TO_CUT_CONNECTION * 1000000 < current_timestamp)
                    cut_connection(tmp->first);
                else if (tmp->second < minimum_timestamp)
                    minimum_timestamp = tmp->second;
            }
        }
        timeout.tv_sec = (minimum_timestamp - current_timestamp + TIMEOUT_TO_CUT_CONNECTION * 1000000) / 1000000;
        timeout.tv_usec = (minimum_timestamp - current_timestamp + TIMEOUT_TO_CUT_CONNECTION * 1000000) % 1000000;
        if (timeout.tv_sec >= TIMEOUT_TO_CUT_CONNECTION)
        {
            timeout.tv_sec = TIMEOUT_TO_CUT_CONNECTION;
            timeout.tv_usec = 0;
        }
    }
}

/*
* load a file from 'path' to match a specific 'route'
*/
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

/*
* accept and store the new connection from the server socket
* adds current timestamp upon successful connection
*/
void server::accept_connection(void)
{
    /* do not accept connection if the backlog is full */
    if (connected_sockets_map.size() >= static_cast<unsigned long>(server_backlog))
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
    if (fcntl(new_socket, F_SETFL, O_NONBLOCK) == -1)
        TERMINATE("fcntl failed");
    FD_SET(new_socket, &connected_sockets);
    connected_sockets_map[new_socket] = get_current_timestamp();
    LOG_TIME("Client joined from socket: " << new_socket);
}

/*
* close and delete 'socket' from the connection list
* TODO: send close response and close connection in stages RFC7230/6.6.
*/
void server::cut_connection(int socket)
{
    /* close socket after we are done communicating */
    close(socket);
    FD_CLR(socket, &connected_sockets);
    connected_sockets_map.erase(socket);
    if (socket == server_socket_fd)
        LOG_TIME("Server disconnected on socket: " << socket);
    else
        LOG_TIME("Client disconnected on socket: " << socket);
}

/* handle the ready to read/write socket
* 1. Parse request
* 2. Format response
* 3. Route to target
*/
void server::handle_connection(int socket)
{
    http_request request_message = parse_request_header(socket);
    if (request_message.reject == true) { /* http_request does not need to be further analyzed, just return a 400 response */
        /* 400 bad request (syntax error) */
        router(socket, request_message);
        return ;
    }
    format_http_request(request_message);
    router(socket, request_message);
    /* RFC7230/6.3. */
    if (request_message.header_fields["Connection"] == "close")
        cut_connection(socket);
    else if (request_message.protocol_version >= "HTTP/1.1")
        /* persistent connection */;
    else if (request_message.protocol_version == "HTTP/1.0")
    {
        if (request_message.header_fields["Connection"] == "keep-alive")
        {
            /* NEED: if recipient is not a proxy */
            /* then persistent connection */
            /* else cut connection */
        }
        else
            cut_connection(socket);
    }
    else
        assert(false); /* protocol version is not properly parsed */
}

/* Message format (RFC7230/3.) -- CURRENTLY FOR REQUEST ONLY
* Fills in http_request object and returns it
* 1. start-line (request-line for request, status-line for response)
* 2. *( header-field CRLF)
* 3. CRLF
* 4. optional message-body/payload (not implemented -- RFC7230/3.3.)
*/
http_request server::parse_request_header(int socket)
{
    http_request message(false);
    /* Read and store request line
    * syntax checking for the request line happens here (RFC7230/3.1.1.)
    */
    std::string current_line = get_next_line(socket);
    if (current_line == CRLF) /* RFC7230/3.5. In the interest of robustness... */
        current_line = get_next_line(socket);
    if (match_pattern(current_line, HEADER_REQUEST_LINE_PATTERN) == false)
        return (http_request::reject_http_request()); /* 400 bad request (syntax error) RFC7230/3.5. last paragraph */
    message.method_token = current_line.substr(0, current_line.find_first_of(' '));
    message.target = current_line.substr(message.method_token.size() + 1);
    message.target = message.target.substr(0, message.target.find_first_of(' '));
    message.protocol_version = current_line.substr(message.method_token.size() + message.target.size() + 2);
    message.protocol_version = message.protocol_version.substr(0, message.protocol_version.find_first_of(CRLF));

    // LOG("Method token: '" << message.method_token << "'");
    // LOG("Target: '" << message.target << "'");
    // LOG("Protocol version: '" << message.protocol_version << "'");

    bool first_header_field = true;
    /* Read and store header fields
    * store each key value pair in 'header_fields'
    * appending field-names that are of the same name happens here (RFC7230/3.2.2.)
    * syntax checking for the header fields happens here
    * if wrong syntax: server must reject the message, proxy should remove the wrongly formatted header field
    * bad (BWS) and optional whitespaces (OWS) are getting removed here
    */
    /* Header field format (RFC7230/3.2.)
    * field-name ":" OWS field-value OWS
    */
    while ((current_line = get_next_line(socket)).size())
    {
        LOG(current_line); 
        if (first_header_field == true) { /* RFC7230/3. A sender MUST NOT send whitespace between the start-line and the first header field */
            if (header_whitespace_characters.count(current_line[0]))
                return (http_request::reject_http_request()); /* 400 bad request (syntax error) */
            first_header_field = false;
        }
        if (current_line == CRLF)
            break ; /* end of header */
        if (match_pattern(current_line, HEADER_FIELD_PATTERN) == false)
            return (http_request::reject_http_request()); /* 400 bad request (syntax error) */
        std::string field_name = current_line.substr(0, current_line.find_first_of(':'));
        std::string field_value_untruncated = current_line.substr(field_name.size() + 1);
        std::string field_value = field_value_untruncated.substr(field_value_untruncated.find_first_not_of(HEADER_WHITESPACES), field_value_untruncated.find_last_not_of(HEADER_WHITESPACES + CRLF));
        if (message.header_fields.count(field_name)) /* append field-name with a preceding comma */
            message.header_fields[field_name] += "," + field_value;
        else
            message.header_fields[field_name] = field_value;
    }
    return (message);
}

/*
* Handle request header fields here
* RFC7231/5.1.
*/
void server::format_http_request(http_request& request)
{
    /* Controls
    * controls are request header fields (key-value pairs) that direct
    * specific handling of the request
    */
    handle_cache_control();
    handle_expect();
    handle_host();
    handle_max_forwards();
    handle_pragma();
    handle_range();
    handle_TE();
    (void)request;
}

/* CONTROLS */

void server::handle_cache_control(void)
{

}

void server::handle_expect(void)
{

}

void server::handle_host(void)
{
}

void server::handle_max_forwards(void)
{

}

void server::handle_pragma(void)
{

}

void server::handle_range(void)
{

}

void server::handle_TE(void)
{

}

/*
* responds to 'socket' based on set parameters
* NEED: constructing http response
*/
void server::router(int socket, const http_request& message)
{
    if (message.reject == true)
    {
        /* respond with 400 */
        std::string response = "HTTP/1.1 200 OK\nContent-Type:text/html\nContent-Length: ";
        response += std::to_string(cachedFiles["/error"].length()) + "\n\n" + cachedFiles["/error"];
        write(socket, response.c_str(), response.length());    
    }
    /* construct response message
    * first line is the Status Line (rfc7230/3.1.2.)
    */
    std::string response = "HTTP/1.1 200 OK\nContent-Type:text/html\nContent-Length: ";
    if (message.target == "/")
        response += std::to_string(cachedFiles["/"].length()) + "\n\n" + cachedFiles["/"];
    else if (message.target == "/about")
        response += std::to_string(cachedFiles["/about"].length()) + "\n\n" + cachedFiles["/about"];
    else if (message.target == "/error" || message.reject == true)
        response += std::to_string(cachedFiles["/error"].length()) + "\n\n" + cachedFiles["/error"];
    else
        response += std::to_string(cachedFiles["/error"].length()) + "\n\n" + cachedFiles["/error"];
    write(socket, response.c_str(), response.length());
}
