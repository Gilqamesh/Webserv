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
    /* http protocol of the server */
    http_version = "HTTP/1.1";
}

void    server::construct(int port, int backlog, unsigned long timestamp, std::map<int, int> *cgiResponses, EventHandler *eventQueue)
{
    server_socket_fd = -1;
    current_number_of_connections = 0;
    server_port = port;
    server_backlog = backlog;
    start_timestamp = timestamp;
    cgi_responses = cgiResponses;
    events = eventQueue;
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

    /* Retrieving client information */
    char buffer[100];
    inet_ntop(AF_INET, &address.sin_addr, buffer, INET_ADDRSTRLEN);
    hostname = buffer;

    /* wait/listen for connection
    * listen(socket, backlog)
    *   the backlog defines the maximum number of pending connections that can be queued up
    *   before connections are refused
    */
    if (listen(server_socket_fd, server_backlog) == -1)
        TERMINATE("listen failed");
    LOG("Server listens on port: " << server_port);
}

/*
* server::add_resource() should be used in the future for this purpose
*
* load a file from 'path' to match a specific 'route' (URL)
* NEED: content-type for resource
* NEED: allowed_methods coming from config file
* NEED: specify if resource is served statically or dynamically, in which case script_path also needs to be provided
* OPTIONAL: content-encoding, content_language, content-location
*/
void server::cache_file(const std::string &path, const std::string &route, bool is_static)
{
    if (cached_resources.count(route))
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
    resource res;
    res.target = route;
    res.content = content;
    /* for now hard-coded, but this needs to be whatever the file's type is */
    res.content_type = "text/html";
    // /* for now hard-coded, but it needs to be the file path relative */
    // res.content_location = path + ".html";
    /* for now hard-coded */
    res.allowed_methods.insert("GET");
    res.allowed_methods.insert("HEAD");
    /* for now hard-coded */
    res.is_static = is_static;
    /* if resource is dynamic, this also has to be provided */
    // res.script_path = ...;
    add_resource(res);
}

void server::add_resource(const resource &resource)
{
    cached_resources[resource.target] = resource;
}

/*
* accept and store the new connection from the server socket
* adds current timestamp upon successful connection
*/
int server::accept_connection(void)
{
    if (current_number_of_connections == server_backlog)
        return (-1);
    ++current_number_of_connections;
    struct sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);
    int new_socket = accept(server_socket_fd, (struct sockaddr *)&addr, &socklen);
    if (new_socket == -1)
        TERMINATE("'accept' failed");

    /* register event for reading on socket */
    events->addReadEvent(new_socket);

	/* register timeout for the socket
    * the idea with this is that this event will be updated on incoming requests
    * so it only triggers if there wasn't a request for the specified time
    */
    events->addTimeEvent(new_socket, TIMEOUT_TO_CUT_CONNECTION * 1000);
    /* turn on non-blocking behavior for this file descriptor
    * any function that would be blocking with this file descriptor will instead not block
    * and return with -1 with errno set to EWOULDBLOCK
    */
    if (fcntl(new_socket, F_SETFL, O_NONBLOCK) == -1)
        TERMINATE("'fcntl' failed");
    // connected_sockets_map.insert(std::pair<int,int>(socket, new_socket));
    // identifyServerSocket.insert(std::pair<int,int>(new_socket, socket));

   LOG_TIME("Client joined from socket: " << new_socket);
   return new_socket;
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
    assert(current_number_of_connections > 0);
    --current_number_of_connections;
    /* close socket after we are done communicating */
    events->removeReadEvent(socket);
    events->removeTimeEvent(socket);
    close(socket);

    if (socket == server_socket_fd)
        LOG_TIME("Server disconnected on socket: " << socket);
    else
        LOG_TIME("Client disconnected on socket: " << socket);
}

/* handle the ready to read/write socket
* 1. Parse request (without body currently)
* 2. Format response
* 3. Send response to 'router' or respond later
* 4. After responding close connection if needed
*/
void server::handle_connection(int socket)
{
    http_request request = parse_request_header(socket);

    struct sockaddr addr;
    socklen_t socklen;
    /* Retrieving client information */
    char buffer[100];
    getsockname(socket, &addr, &socklen);
    inet_ntop(AF_INET, &((struct sockaddr_in *)&addr)->sin_addr, buffer, INET_ADDRSTRLEN);
    request.hostname = buffer;
    request.port = std::to_string(ntohs(((struct sockaddr_in *)&addr)->sin_port));

    format_http_request(request);
    http_response response = format_http_response(request);
    if (response.reject == false)
        router(socket, response);
    if (request.reject == true)
        cut_connection(socket);
}

/* Request format (RFC7230/3.)
* Fills in http_request object and returns it
* 1. start-line (request-line for request, status-line for response)
* 2. *( header-field CRLF)
* 3. CRLF
* 4. optional request-body/payload
*
* Construct absolute URI RFC2396/3.
*/
http_request server::parse_request_header(int socket)
{
    http_request request(false);
    request.socket = socket;
    /* Read and store request line
    * syntax checking for the request line happens here (RFC7230/3.1.1.)
    */
    std::string current_line = get_next_line(socket);
    if (current_line == CRLF) /* RFC7230/3.5. In the interest of robustness... */
        current_line = get_next_line(socket);
    if (match_pattern(current_line, HEADER_REQUEST_LINE_PATTERN) == false)
        return (http_request::reject_http_request()); /* 400 bad request (syntax error) RFC7230/3.5. last paragraph */
    request.scheme = "http";
    request.method_token = current_line.substr(0, current_line.find_first_of(' '));
    request.target = current_line.substr(request.method_token.size() + 1);
    request.target = request.target.substr(0, request.target.find_first_of(' '));
    request.protocol_version = current_line.substr(request.method_token.size() + request.target.size() + 2);
    request.protocol_version = request.protocol_version.substr(0, request.protocol_version.find_first_of(CRLF));

    // LOG("Method token: '" << request.method_token << "'");
    // LOG("Target: '" << request.target << "'");
    // LOG("Protocol version: '" << request.protocol_version << "'");

    bool first_header_field = true;
    /* Read and store header fields
    * store each key value pair in 'header_fields'
    * appending field-names that are of the same name happens here (RFC7230/3.2.2.)
    * syntax checking for the header fields happens here
    * if wrong syntax: server must reject the request, proxy should remove the wrongly formatted header field
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
        if (field_name == "Host" && request.header_fields.count(field_name))
            return (http_request::reject_http_request()); /* RFC7230/5.4. */
        std::string field_value_untruncated = current_line.substr(field_name.size() + 1);
        std::string field_value = field_value_untruncated.substr(field_value_untruncated.find_first_not_of(HEADER_WHITESPACES), field_value_untruncated.find_last_not_of(HEADER_WHITESPACES + CRLF));
        if (request.header_fields.count(field_name)) /* append field-name with a preceding comma */
            request.header_fields[field_name] += "," + field_value;
        else
            request.header_fields[field_name] = field_value;
    }
    if (request.header_fields.count("Host") == 0)
        return (http_request::reject_http_request());
    request.abs_path = request.target.substr(0, request.target.find_first_of('?'));
    request.net_path = "//" + request.header_fields["Host"] + request.abs_path;
    if (request.target.find_first_of('?') != std::string::npos)
        request.query = request.target.substr(request.target.find_first_of('?') + 1);
    request.URI = request.scheme + ":" + request.net_path;
    if (request.query.length())
        request.URI += "?" + request.query;
    // LOG("Abs path: " << request.abs_path);
    // LOG("Net path: " << request.net_path);
    // LOG("Query: " << request.query);
    // LOG("URI: " << request.URI);
    while ((current_line = get_next_line(socket)).size())
        request.payload += current_line;
    // LOG("Payload: " << request.payload);
    return (request);
}

/*
* Handle request header fields here RFC7231/5.1.
*/
void server::format_http_request(http_request& request)
{
    /* Controls
    * controls are request header fields (key-value pairs) that direct
    * specific handling of the request
    */
    request_control_cache_control(request);
    request_control_expect(request);
    request_control_host(request);
    request_control_max_forwards(request);
    request_control_pragma(request);
    request_control_range(request);
    request_control_TE(request);

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

/* REQUEST CONTROLS */

void            server::request_control_cache_control(http_request &request)
{
    (void)request;
}

void            server::request_control_expect(http_request &request)
{
    (void)request;
}

void            server::request_control_host(http_request &request)
{
    (void)request;
}

void            server::request_control_max_forwards(http_request &request)
{
    (void)request;
}

void            server::request_control_pragma(http_request &request)
{
    (void)request;
}

void            server::request_control_range(http_request &request)
{
    (void)request;
}

void            server::request_control_TE(http_request &request)
{
    (void)request;
}


/* Constructs http_response
* 1. Construct Status Line
*   1.1. HTTP-version
*   1.2. Status Code
*   1.3. Reason Phrase
* 2. Construct Header Fields using Control Data RFC7231/7.1.
* 3. Construct Message Body
*       either server static resource
*       or execute script with CGI to serve dynamically constructed resource
*/
http_response server::format_http_response(const http_request& request)
{
    http_response response;
    response.http_version = http_version;
    if (request.reject == true) { /* Bad Request */
        response.status_code = "400";
        response.reason_phrase = "Bad Request";
    } else if (cached_resources.count(request.target) == 0) { /* Not Found */
        response.status_code = "404";
        response.reason_phrase = "Not Found";
    } else if (accepted_request_methods.count(request.method_token) == 0) { /* Not Implemented */
        response.status_code = "501";
        response.reason_phrase = "Not Implemented";
    } else if (cached_resources[request.target].allowed_methods.count(request.method_token) == 0) { /* Not Allowed */
        response.status_code = "405";
        response.reason_phrase = "Method Not Allowed";
        /* RFC7231/6.5.5. must generate Allow header field */
        for (std::unordered_set<std::string>::const_iterator cit = cached_resources[request.target].allowed_methods.begin(); cit != cached_resources[request.target].allowed_methods.end(); ++cit)
            response.header_fields["Allow"] += response.header_fields.count("Allow") ? "," + *cit : *cit;
    } else if (request.method_token == "POST") { /* Target resource needs to process the request */
        /* RFC7231/4.3.3.
        * CGI for example comes here
        * if one or more resources has been created, status code needs to be 201 with "Created"
        *   with "Location" header field that provides an identifier for the primary resource created
        *   and a representation that describes the status of the request while referring to the new resource(s).
        * If no resources are created, respond with 200 that containts the result and a "Content-Location"
        *   header field that has the same value as the POST's effective request URI
        * If result is equivalent to already existing resource, redirect with 303 with "Location" header field
        */
    } else { /* 200 */
        response.status_code = "200";
        response.reason_phrase = "OK";
    }

    /* Control Data RFC7231/7.1. */
    response_control_handle_age(response);
    response_control_cache_control(response);
    response_control_expires(response);
    response_control_date(response);
    response_control_location(response);
    response_control_retry_after(response);
    response_control_vary(response);
    response_control_warning(response);

    representation_metadata(request, response);

    payload_header_fields(request, response);

    /* add payload */
    if (match_pattern(response.status_code, "4..") == false && cached_resources[request.target].is_static == false) /* if resource exists and is dynamic */
    {
        /* NEED: execute the corresponding script with CGI
        * 1. pass meta_variables as environment variables to the script
        * 2. fork and execute script
        * 3. retrieve output at some point somehow
        *   we can probably set up a socket which is inherited to the script - which will
        *   be the communication between server and the CGI - and then add an event for it
        *   so that it can be later retrieved from the event queue,
        *   after which we can respond to the client.. if the event passes a certain timeout,
        *   kill the process and handle client accordingly
        */
        int cgi_pipe[2];
        if (pipe(cgi_pipe) == -1)
            TERMINATE("pipe failed");
        if (fcntl(cgi_pipe[READ_END], F_SETFL, O_NONBLOCK) == -1)
            TERMINATE("fcntl failed");
        if (fcntl(cgi_pipe[WRITE_END], F_SETFL, O_NONBLOCK) == -1)
            TERMINATE("fcntl failed");
        /* write request payload into socket */
        (*cgi_responses)[cgi_pipe[READ_END]] = request.socket; /* to serve client later */
        CGI script(cgi_pipe, request.payload);
        add_script_meta_variables(script, request);
        script.execute();
        close(cgi_pipe[WRITE_END]);
        /* register an event for this socket */
        events->addReadEvent(cgi_pipe[READ_END], (int *)&cgi_responses->find(cgi_pipe[READ_END])->first);
        return (http_response::reject_http_response());
    }
    if (match_pattern(response.status_code, "2..") == true)
    {
        if (response.header_fields.count("Content-Length"))
            response.payload = cached_resources[request.target].content.substr(0, std::atoi(response.header_fields["Content-Length"].c_str()));
        else
            response.payload = cached_resources[request.target].content;
    }
    else
        response.payload = cached_resources["/error"].content;

    return (response);
}

/* RESPONSE CONTROLS */

void            server::response_control_handle_age(http_response &response)
{
    (void)response;
}

void            server::response_control_cache_control(http_response &response)
{
    (void)response;
}

void            server::response_control_expires(http_response &response)
{
    (void)response;
}

void            server::response_control_date(http_response &response)
{
    (void)response;
}

void            server::response_control_location(http_response &response)
{
    (void)response;
}

void            server::response_control_retry_after(http_response &response)
{
    (void)response;
}

void            server::response_control_vary(http_response &response)
{
    (void)response;
}

void            server::response_control_warning(http_response &response)
{
    (void)response;
}

/* Header fields for payload RFC7231/3.1.
* sets header fields to provide metadata about the representation
*/
void server::representation_metadata(const http_request &request, http_response &response)
{
    if (cached_resources.count(request.target) == 0)
    {
        response.header_fields["Content-Type"] = "text/html";
        response.header_fields["Content-Language"] = "en-US";
        return ;
    }
    response.header_fields["Content-Type"] = cached_resources[request.target].content_type;
    for (std::unordered_set<std::string>::const_iterator cit = cached_resources[request.target].content_encoding.begin(); cit != cached_resources[request.target].content_encoding.end(); ++cit)
        response.header_fields["Content-Encoding"] += response.header_fields.count("Content-Encoding") ? "," + *cit : *cit;
    for (std::unordered_set<std::string>::const_iterator cit = cached_resources[request.target].content_language.begin(); cit != cached_resources[request.target].content_language.end(); ++cit)
        response.header_fields["Content-Language"] += response.header_fields.count("Content-Language") ? "," + *cit : *cit;
    if (cached_resources[request.target].content_location.length())
        response.header_fields["Content-Location"] = cached_resources[request.target].content_location;
}

void server::representation_metadata(http_request &request)
{
    (void)request;
}

/* Payload Semantics RFC7231/3.3.
* 1. Content-Length
*   'Transfer-Encoding' header field must be known at this point
* 2. Content-Range: NOT IMPLEMENTED
* 3. Trailer: NOT IMPLEMENTED
*/
void server::payload_header_fields(const http_request &request, http_response &response)
{
    if (match_pattern(response.status_code, "[45]..") == true)
    {
        response.header_fields["Content-Length"] = std::to_string(cached_resources["/error"].content.length());
        return ;
    }
    if (response.header_fields.count("Transfer-Encoding") == 0) /* set up Content-Length */
    {
        if (response.status_code == "204" || match_pattern(response.status_code, "1..") == true)
            /* No Content-Length header field */;
        else
            response.header_fields["Content-Length"] = std::to_string(cached_resources[request.target].content.length());
    }
    if (response.status_code == "204" || match_pattern(response.status_code, "1..") == true
        || (request.method_token == "CONNECT" && match_pattern(response.status_code, "2..") == true))
        /* No Transfer-Encoding header field */;
    else
        for (std::unordered_set<std::string>::const_iterator cit = cached_resources[request.target].content_encoding.begin(); cit != cached_resources[request.target].content_encoding.end(); ++cit)
            response.header_fields["Transfer-Encoding"] += response.header_fields.count("Transfer-Encoding") ? "," + *cit : *cit;
}

/* responds to 'socket' based on set parameters
* 1. Construct http response
* 2. Send response
* Warning: The message might not fit into the 'send' buffer which is not handled
*/
void server::router(int socket, const http_response &response)
{
    // LOG("Request target: " << request.target);
    // LOG("Status-line: " << response.http_version << " " << response.status_code << " " << response.reason_phrase);
    // LOG("Header fields");
    // for (std::unordered_map<std::string, std::string>::const_iterator cit = response.header_fields.begin(); cit != response.header_fields.end(); ++cit)
    //     LOG(cit->first << ": " << cit->second);
    // LOG("Payload: " << response.payload);

    std::string message = response.http_version + " " + response.status_code + " " + response.reason_phrase + "\n";
    for (std::unordered_map<std::string, std::string>::const_iterator cit = response.header_fields.begin(); cit != response.header_fields.end(); ++cit)
        message += cit->first + ":" + cit->second + "\n";
    message += "\n";
    message += response.payload;
    send(socket, message.c_str(), message.length(), 0);
}

void server::send_timeout(int socket)
{
    http_response response;
    response.http_version = http_version;
    response.status_code = "408";
    response.reason_phrase = "Request Timeout";
    response.header_fields["Connection"] = "close";
    response.payload = cached_resources["/error"].content;
    router(socket, response);
}

int const       &server::getServerSocketFd(void) const {return this->server_socket_fd;}
/* Add request meta-variables to the script RFC3875/4.1. */
void server::add_script_meta_variables(CGI &script, const http_request &request)
{
    // script.add_meta_variable("AUTH_TYPE", "");
    script.add_meta_variable("CONTENT_LENGTH", std::to_string(request.payload.length()));
    if (request.header_fields.count("Content-Type"))
        script.add_meta_variable("CONTENT_TYPE", request.header_fields.at("Content-Type"));
    script.add_meta_variable("PATH_INFO", request.abs_path);
    char *cwd;
    if ((cwd = getcwd(NULL, 0)) == NULL)
        TERMINATE("getcwd failed in 'add_script_meta_variables'");
    /* This should be handled from the calling server */
    script.add_meta_variable("PATH_TRANSLATED", cwd + request.abs_path);
    script.add_meta_variable("QUERY_STRING", request.query);
    script.add_meta_variable("REMOTE_ADDR", request.hostname);
    script.add_meta_variable("REMOTE_HOST", request.hostname);
    /* if AUTH_TYPE is set then
    * script.add_meta_variable("REQUEST_USER", "");
    */
    script.add_meta_variable("REQUEST_METHOD", request.method_token);
    script.add_meta_variable("SCRIPT_NAME", request.abs_path);
    script.add_meta_variable("SERVER_NAME", this->hostname);
    script.add_meta_variable("SERVER_PORT", std::to_string(this->server_port));
    script.add_meta_variable("SERVER_PROTOCOL", this->http_version);
    /* name/version of the server, no clue what this means currently.. */
    // script.add_meta_variable("SERVER_SOFTWARE", "");
    for (std::unordered_map<std::string, std::string>::const_iterator cit = request.header_fields.begin(); cit != request.header_fields.end(); ++cit)
    {
        if (cit->first == "Authorization" || cit->first == "Content-Length" || cit->first == "Content-Type"
            || cit->first == "Connection")
            continue ;
        std::string key = "HTTP_" + to_upper(cit->first);
        std::replace(key.begin(), key.end(), '-', '_');
        script.add_meta_variable(key, cit->second);
    }
}
