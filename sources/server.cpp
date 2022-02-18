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
    // connected_sockets_map.insert(std::make_pair<int, unsigned long>(server_socket_fd, get_current_timestamp()));
    connected_sockets_set.insert(server_socket_fd);

    // the kqueue holds all the events we are interested in
    // to start, we simply create an empty kqueue
    if ((this->kq = kqueue()) == -1)
        TERMINATE("failed to create empty kqueue");
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

    // EV_SET() -> initialize a kevent structure, here our listening server
    EV_SET(&evSet, server_socket_fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);

    // ...and register it to the kqueue;
    if (kevent(kq, &evSet, 1, NULL, 0, 0) == -1)
        TERMINATE("Registration failed");

    while (true)
    {
        // call kevent(..) to receive incoming events and process them
        // waiting/reading events (up to N (N = MAX_EVENTS) at a time)
        // returns number of events
        int nev = kevent(kq, NULL, 0, evList, MAX_EVENTS, NULL);

        for (int i = 0; i < nev; ++i)
        {
            int fd = static_cast<int>(evList[i].ident);
            if (fd == server_socket_fd)                 // receive new connection
                accept_connection(fd);
            else if (evList[i].filter == EVFILT_READ)
            {
                if (evList[i].flags & EV_EOF)          // client disconnected
                    cut_connection(fd);
                else
                    handle_connection(fd);
            }
            else if (evList[i].filter == EVFILT_WRITE)
            {
                // TODO -> handle EVFILT_WRITE
            }
            else if (evList[i].filter == EVFILT_TIMER)  // check for timeout
                cut_connection(fd);
            else if (evList[i].flags & EV_EOF)          // client disconnected
                cut_connection(fd);
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
void server::accept_connection(int socket)
{
    /* do not accept connection if the backlog is full */
    if (connected_sockets_set.size() >= static_cast<unsigned long>(server_backlog))
        return ;
    struct sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);
    int new_socket = accept(socket, (struct sockaddr *)&addr, &socklen);
    if (new_socket == -1)
        TERMINATE("accept failed");

    // Adding events we are interested in:

    // register listening handler
    EV_SET(&evSet, new_socket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
    kevent(kq, &evSet, 1, NULL, 0, NULL);

    // register writing handler
    EV_SET(&evSet, new_socket, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ONESHOT, 0, 0, NULL);
    kevent(kq, &evSet, 1, NULL, 0, NULL);

	// register timeout handler
    EV_SET(&evSet, new_socket, EVFILT_TIMER, EV_ADD | EV_CLEAR | EV_ONESHOT, 0, 5000, NULL);
    kevent(kq, &evSet, 1, NULL, 0, NULL);

    if (fcntl(new_socket, F_SETFL, O_NONBLOCK) == -1)
        TERMINATE("fcntl failed");
	connected_sockets_set.insert(new_socket);
    LOG_TIME("Client joined from socket: " << new_socket);
}

/*
* close and delete 'socket' from the connection list
// for the below problem the shutdown() function would be a solution but it's not allowed
// * To avoid TCP reset problem (RFC7230/6.6) the connection is closed in stages
// *   first, half-close by closing only the write side of the read/write connection
// *   then continue to read from the connection until receiving a corresponding close by the client
// *       or until we are reasonably certain that the client has received the server's last response
// *   lastly, fully close the connection
*/
void server::cut_connection(int socket)
{
    /* close socket after we are done communicating */
    close(socket);
    // Socket is automatically removed from the kq by the kernel.
    connected_sockets_set.erase(socket);
    if (socket == server_socket_fd)
        LOG_TIME("Server disconnected on socket: " << socket);
    else
        LOG_TIME("Client disconnected on socket: " << socket);
}

/* handle the ready to read/write socket
* 1. Parse request
* 2. Format response
* 3. Send response in 'router'
* 4. After responding close connection if needed
*/
void server::handle_connection(int socket)
{
    http_request request_message = parse_request_header(socket);
    format_http_request(request_message);
    router(socket, request_message);
    if (request_message.reject == true)
        cut_connection(socket);
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
        // LOG(current_line); 
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

    /* RFC7230/6.3. */
    if (request.reject == true) /* if 'request' is rejected, no need to further analyse */
        return ;
    if (request.header_fields["Connection"] == "close")
        request.reject = true;
    else if (request.protocol_version >= "HTTP/1.1")
        /* persistent connection */;
    else if (request.protocol_version == "HTTP/1.0")
    {
        if (request.header_fields["Connection"] == "keep-alive")
        {
            /* NEED: if recipient is not a proxy */
            /* then persistent connection */
            /* else cut connection */
        }
        else
            request.reject = true;
    }
    else
        assert(false); /* protocol version is not properly parsed or a case is not handled */
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
