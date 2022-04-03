#include "server.hpp"
#include "utils.hpp"
#include <cstdio>
#define READ_HTTP_BUFFER_SIZE 4194304

/*
* helper to initialize some constants
*/

void    server::read_request(int fd)
{   
    std::vector<char> tmp(READ_HTTP_BUFFER_SIZE);
    nOfBytesRead = recv(fd, tmp.data(), READ_HTTP_BUFFER_SIZE, 0);
    NETWORK_LOG("nOfBytesRead: " << nOfBytesRead);
    for (size_t i = 0; i < nOfBytesRead; ++i)
        main_vec.push_back(tmp[i]);
    tmp.clear();
}

void    server::get_header_fields(void)
{
    headerFields.clear();
    std::string tmp;
    for (size_t i = 0; i < main_vec.size(); ++i)
    {
        tmp += main_vec[i];
        if (main_vec[i] == '\n')
        {
            headerFields.push_back(tmp);
            tmp.clear();
            if (headerFields.back() == "\r\n")
            {
                start_body_pos = i++;

                for (int j = 0; headerFields[j] != "\r\n"; ++j)
                {
                    while (headerFields[j].back() == '\r' || headerFields[j].back() == '\n')
                        headerFields[j].pop_back();
                }
                header_is_parsed = true;
                PRINT_HERE();
                return ;
            }
        }
    }
}

bool    server::get_header_infos(void)
{
    for (std::vector<std::string>::iterator it = headerFields.begin(); it != headerFields.end(); ++it)
    {
        if ((*it).find("POST") != std::string::npos)
            is_post = true;
        else if ((*it).find("Transfer-Encoding: chunked") != std::string::npos)
            chunked = true;
        else if ((*it).find("Content-Length") != std::string::npos)
        {
            found_content_length = true;
            if (server_configuration.client_max_body_size != -1
                && stoi(conf_file::get_words(*it)[1]) > server_configuration.client_max_body_size)
                return (false);
        }
    }
    return (true);
}

void    server::get_body(void)
{
    request_body.clear();
    if (finished_reading == true)
    {
        for (; start_body_pos != main_vec.size(); ++start_body_pos)
            request_body += main_vec[start_body_pos];
    }
    else
    {
        for (; start_body_pos != nOfBytesRead; ++start_body_pos)
            request_body += main_vec[start_body_pos];
    }
}

http_request server::parse_request_header(int socket)
{  
    read_request(socket);
    if (header_is_parsed == false)
    {
        get_header_fields();
        if (header_is_parsed == false)
            return (http_request::chunked_http_request());
        if (!get_header_infos())
            return (http_request::reject_http_request());
        if (is_post == true && chunked == false && found_content_length == false)
        {
            PRINT_HERE();
            return (http_request::reject_http_request());
        }
    }
    if (chunked == true)
    {
        std::vector<char>::iterator it = main_vec.end();
        // LOG("ASCII");
        // for (std::vector<char>::iterator it2 = main_vec.begin(); it2 != main_vec.end(); ++it2)
        //     printf("[%d] ", *it2);
        // LOG("");
        if (*(it - 1) == '\n' && *(it - 2) == '\r' && *(it - 3) == '\n' && *(it - 4) == '\r' && *(it - 5) == '0')
        {
            finished_reading = true;
            get_body();
        }
    }
    else
    {
        if (nOfBytesRead < READ_HTTP_BUFFER_SIZE)
            finished_reading = true;
        get_body();
    }
    if (finished_reading == false)
        return (http_request::chunked_http_request());
    main_vec.clear();
    start_body_pos = 0;

    http_request request(false);
    request.socket = socket;
    if (chunked == true)
    {
        request.payload = decoding_chunked(request_body);
        if (server_configuration.client_max_body_size != -1
            && chunks_size > server_configuration.client_max_body_size)
            return (http_request::reject_http_request());
    }
    else
        request.payload = request_body;

    // LOG("request_body = \n" << request_body);
    // LOG("request.payload = \n" << request.payload);

    if (check_first_line(headerFields[0], request) == false)
    {
        PRINT_HERE();
        return (http_request::reject_http_request());
    }

    size_t i = 1;
    std::string current_line;
    while (true)
    {
        if (headerFields[i] == "\r\n" || headerFields[i] == "\n" || headerFields[i] == "\r")
            break ; /* end of header */
        if (check_prebody(headerFields[i], request) == false)
        {
            PRINT_HERE();
            return (http_request::reject_http_request());
        }
        ++i;
    }

    if (request.header_fields.count("Host") == 0)
    {
        PRINT_HERE();
        return (http_request::reject_http_request());
    }
    request.abs_path = request.target.substr(0, request.target.find_first_of('?'));
    request.net_path = "//" + request.header_fields["Host"] + request.abs_path;
    if (request.target.find_first_of('?') != std::string::npos)
        request.query = request.target.substr(request.target.find_first_of('?') + 1);
    request.URI = request.scheme + ":" + request.net_path;
    if (request.query.length())
        request.URI += "?" + request.query;
    LOG(displayTimestamp() << " REQUEST  -> [method: " << request.method_token << "] [target: " << request.target << "] [version: " << request.protocol_version << "]");
    NETWORK_LOG("Log ID: " << network_log_id++ << "\n" << displayTimestamp() << " REQUEST  -> [method: " << request.method_token << "] [target: " << request.target << "] [version: " << request.protocol_version << "]");
    return (request);
}

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

void    server::construct(int port, int backlog, unsigned long timestamp, std::map<int, int> *cgiResponses, EventHandler *eventQueue,
const t_server &configuration)
{
    server_socket_fd = -1;
    current_number_of_connections = 0;
    server_port = port;
    server_backlog = backlog;
    start_timestamp = timestamp;
    cgi_responses = cgiResponses;
    events = eventQueue;
    server_configuration = configuration;
    locations = configuration.locations;
    finished_reading = false;
    header_is_parsed = false;
    found_content_length = false;
    is_post = false;
    content_length = 0;
    chunked = false;
    start_body_pos = 0;
    chunks_size = 0;
    for (unsigned int i = 0; i < locations.size(); ++i)
    {
        // LOG("Redirect for route " + locations[i].route + ": " + locations[i].redirect);
        std::string newRoute = locations[i].route;
        if (newRoute.back() == '/')
            newRoute = newRoute.substr(0, newRoute.size() - 1);
        sortedRoutes[newRoute] = locations[i];
    }
	initialize_constants();
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1)
        TERMINATE("failed to create a socket");
    
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &server_socket_fd, sizeof(int)) == -1)
        TERMINATE("setsockopt failed");
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

    if (listen(server_socket_fd, server_backlog) == -1)
        TERMINATE("listen failed");
    // LOG("Server listens on port: " << server_port);
    LOG(displayTimestamp() << " Server listens on port: " << server_port);

    cache_file();
    resource error_page;
    error_page.target = "/error";
    error_page.path = "./views/error.html";
    if (configuration.error_page.size())
        error_page.path = configuration.error_page;
    error_page.content_type = "text/html";
    std::ifstream ifs(error_page.path);
    if (!ifs)
        WARN("Could not open " + error_page.path);
    std::string tmp;
    while (std::getline(ifs, tmp))
        error_page.content += tmp;
    error_page.is_static = true;
    /* TODO: write error page CGI script */
    // error_page.script_path = "";

    add_resource(error_page);
}

void server::cache_file()
{
    for (unsigned int i = 0; i < locations.size(); ++i)
    {
        const t_location &cur_location = locations[i];
        if (cached_resources.count(cur_location.route))
        {
            WARN("route: '" << cur_location.route << "' already exists");
            return ;
        }
        const std::string &path = cur_location.root + "/" + cur_location.index;
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
        res.target = cur_location.route;
        if (res.target.back() != '/')
            res.target += "/";
        res.content = content;
        res.path = path;
        /* for now hard-coded, but this needs to be whatever the file's type is */
        res.content_type = "text/html";
        // /* for now hard-coded, but it needs to be the file path relative */
        // res.content_location = path + ".html";
        for (unsigned int i = 0; i < cur_location.methods.size(); ++i)
            res.allowed_methods.insert(cur_location.methods[i]);
        /* for now hard-coded */
        res.is_static = true;
        /* if resource is dynamic, this also has to be provided */
        // res.script_path = ...;
        add_resource(res);
    }
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
    events->addReadEvent(new_socket);
    events->addTimeEvent(new_socket, TIMEOUT_TO_CUT_CONNECTION * 1000);
    if (fcntl(new_socket, F_SETFL, O_NONBLOCK) == -1)
        TERMINATE("'fcntl' failed");
    LOG(displayTimestamp() << " Client joined from socket: " << new_socket);
    NETWORK_LOG(displayTimestamp() << " Client joined from socket: " << new_socket);
   return new_socket;
}

void server::cut_connection(int socket)
{
    LOG("Socket: " << socket);
    assert(current_number_of_connections > 0);
    --current_number_of_connections;
    /* close socket after we are done communicating */
    events->removeReadEvent(socket);
    events->removeTimeEvent(socket);
    close(socket);

    if (socket == server_socket_fd)
    {
        LOG(displayTimestamp() << " Server disconnected on socket: " << socket);
        NETWORK_LOG(displayTimestamp() << " Server disconnected on socket: " << socket << std::endl);
    }
    else
    {
        LOG(displayTimestamp() << " Client disconnected on socket: " << socket);
        NETWORK_LOG(displayTimestamp() << " Client disconnected on socket: " << socket << std::endl);
    }
}

void server::handle_connection(int socket)
{
    http_request request = parse_request_header(socket);
    if (request.reject == false && finished_reading == false)
        return ;
    LOG("Request target: " << request.target);
    std::string redirectionStr = isAllowedDirectory3(request.target);
    LOG("isAllowedDirectory3(request.target): " << redirectionStr);
    if (redirectionStr.size()) { /* redirect client to reformatted target */
        request.redirected = true;
        request.target = redirectionStr;
    } else {
        request.redirected = false;
    }
    std::string httpRequest;
    for (std::vector<std::string>::iterator it = headerFields.begin(); it != headerFields.end(); ++it)
        httpRequest += *it;
    httpRequest += request_body;
    HTTP_MESSAGE_LOG("\n\n\n\nLog ID: " << http_message_log_id++ << "\n------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------"
        << std::endl << displayTimestamp() << " [REQUEST from socket - " << socket << "]" << std::endl
        << httpRequest << std::endl
        << "------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    if (is_post == true && found_content_length == false && chunked == false)
    {
        PRINT_HERE();
        http_response response = http_response::reject_http_response();
        router(socket, response);
        cut_connection(socket);
        return ;
    }
    finished_reading = false; 
    header_is_parsed = false;
    chunked = false;
    is_post = false;
    request_body.clear();
    struct sockaddr addr;
    socklen_t socklen;
    /* Retrieving client information */
    char buffer[100];
    getsockname(socket, &addr, &socklen);
    inet_ntop(AF_INET, &((struct sockaddr_in *)&addr)->sin_addr, buffer, INET_ADDRSTRLEN);
    request.hostname = buffer;
    request.port = std::to_string(ntohs(((struct sockaddr_in *)&addr)->sin_port));

    http_response response = format_http_response(request);
    PRINT_HERE();
    headerFields.clear();
    PRINT_HERE();
    LOG("response.handled_by_cgi: " << response.handled_by_cgi);
    if (response.handled_by_cgi == false)
        router(socket, response);
    PRINT_HERE();
    // if (request.reject == true)
    //     cut_connection(socket);
}

bool    server::check_first_line(std::string current_line, http_request &request)
{
    LOG("current line: " << current_line);
    if (match(current_line, HEADER_REQUEST_LINE_PATTERN) == false)
    {
        PRINT_HERE();
        return (false); /* 400 bad request (syntax error) RFC7230/3.5. last paragraph */
    }
    request.scheme = "http";
    std::vector<std::string> words = conf_file::get_words(current_line);
    request.method_token = words[0];
    request.target = words[1];
    request.protocol_version = words[2];

    std::string constructedTarget(request.target);
    constructedTarget = isAllowedDirectory(request.target);
    if (constructedTarget.size() && fileExists(constructedTarget) == 2 && request.target.back() != '/')
        request.target += "/";
    return true;
}

bool            server::check_prebody(std::string current_line,  http_request &request)
{
    bool    first_header_field = 1;

    // std::cout << "header field: " << current_line;
    if (first_header_field == true) { /* RFC7230/3. A sender MUST NOT send whitespace between the start-line and the first header field */
        if (header_whitespace_characters.count(current_line[0]))
            return (0); /* 400 bad request (syntax error) */
        first_header_field = false;
    }
    if (match(current_line, HEADER_FIELD_PATTERN) == false)
        return (0); /* 400 bad request (syntax error) */
    std::string field_name = current_line.substr(0, current_line.find_first_of(':'));
    if (field_name == "Host" && request.header_fields.count(field_name))
        return (0); /* RFC7230/5.4. */
    std::string field_value_untruncated = current_line.substr(field_name.size() + 1);
    // LOG("field_value_untruncated: " << field_value_untruncated);
    std::string field_value = field_value_untruncated.substr(field_value_untruncated.find_first_not_of(HEADER_WHITESPACES), field_value_untruncated.find_last_not_of(HEADER_WHITESPACES + CRLF));
    if (request.header_fields.count(field_name)) /* append field-name with a preceding comma */
        request.header_fields[field_name] += "," + field_value;
    else
        request.header_fields[field_name] = field_value; 
    return true;
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

std::string server::isAllowedDirectory(const std::string &target)
{
    for (std::map<std::string, t_location>::const_reverse_iterator cit = sortedRoutes.rbegin(); cit != sortedRoutes.rend(); ++cit)
    {
        if (target.substr(0, cit->first.size()) == cit->first)
        {
            /* construct string */
            std::string path = cit->second.root + "/";
            // LOG("path: " << path);
            // LOG("cit->first: " << cit->first);
            path += target.substr((target[cit->first.size() - 1] == '/' ? cit->first.size() + 1 : cit->first.size()));
            // path += target.substr(cit->first.size());
            // /directory
            // LOG("path: " << path);
            /* remove route from the beginning of 'target'
             * add root to the beginning of target
            */

            // if (target == cit->second.route.substr(0, cit->second.route.find_last_of("/")))
            //     return (path);
            int ret = fileExists(path);
            if (ret == 2) /* if its a directory */
            {
                for (unsigned int i = 0; i < locations.size(); i++)
                {
                    if (locations[i].route.back() == '/')
                        locations[i].route.pop_back();
                    if (locations[i].route == cit->first)
                    {
                        LOG("HHHHHHHHHHHHHHH");
                        if (locations[i].autoindex == 0)
                        {
                            LOG("HMMMMMMMMMMMM");
                            return ("");
                        }
                    }
                }
                std::string subDirIndex = path + (path.back() == '/' ? "" : "/") + cit->second.index;
                // LOG("Index - " << subDirIndex);
                int ret2 = fileExists(subDirIndex);
                if (ret2 == 1) /* if the index file exists */
                    return (subDirIndex);
                else
                    return ("");
            }
            else if (ret == 1) /* if its a file */
                return (path);
            else /* either directory or does not exist */
                return ("");
        }
    }
    return ("");
}

/*
 * Returns substituted path if 'target' is under any of our 'location' directories
 */
std::string server::isAllowedDirectory2(const std::string &target)
{
    for (std::map<std::string, t_location>::const_reverse_iterator cit = sortedRoutes.rbegin(); cit != sortedRoutes.rend(); ++cit)
    {
        // cit->second.root == views
        // cit->first == /error
        // target == /error/lol/asd.txt
        // path = views/lol/asd.txt
        if (cit->first == "")
            return ("");
        if (target.substr(0, cit->first.size()) == cit->first)
        {
            std::string path = cit->second.root;
            if (target.size() > cit->first.size())
                path += target.substr(cit->first.size());
            return (path);
        }
    }
    return ("");
}

/*
 * Returns substituted path if 'target' needs to be redirected
 * Empty str if doesn't need to be redirected
 */
std::string server::isAllowedDirectory3(const std::string &target)
{
    for (std::map<std::string, t_location>::const_reverse_iterator cit = sortedRoutes.rbegin(); cit != sortedRoutes.rend(); ++cit)
    {
        // cit->second.redirect == /location2
        // cit->first == /error
        // target == /error/lol/asd.txt
        // path = location2/lol/asd.txt
        if (cit->first == "")
            return ("");
        if (target.substr(0, cit->first.size()) == cit->first)
        {
            std::string path = cit->second.redirect;
            if (path.size() == 0)
                return ("");
            if (path.front() == '/')
                path = path.substr(1);
            if (target.size() > cit->first.size())
                path += target.substr(cit->first.size());
            return (path);
        }
    }
    return ("");
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

/* Constructs http_responsee
* 1. Construct Status Line
*   1.1. HTTP-version
*   1.2. Status Code
*   1.3. Reason Phrase
* 2. Construct Header Fields using Control Data RFC7231/7.1.
* 3. Construct Message Body
*       either server static resource
*       or execute script with CGI to serve dynamically constructed resource
*/
http_response server::format_http_response(http_request& request)
{
    http_response response;
    response.http_version = http_version;

    if (request.method_token == "POST" || request.method_token == "PUT")
    {
        size_t pos = request.target.find_last_of('.');
        std::string extension;
        if (pos != std::string::npos)
            extension = request.target.substr(pos);
        request.extension = server_configuration.general_cgi_path;
        if (server_configuration.general_cgi_extension.size() && extension == server_configuration.general_cgi_extension)
            return (handle_post_request(request));
    }

    if (request.reject == true) { /* Bad Request */
        response.status_code = "400";
        response.reason_phrase = "Bad Request";
        response.header_fields["Connection"] = "close";
    } else if (cached_resources.count(request.target) == 0) { /* Not Found */
        if (request.method_token == "POST" || request.method_token == "PUT")
            return (handle_post_request(request));
        std::string constructedPath(isAllowedDirectory(request.target));
        if (constructedPath.size()) { /* check if directory is allowed and if the file exists */

            cached_resources[request.target].allowed_methods.insert("GET");
            cached_resources[request.target].is_static = true;
            /* add this to the cached_resources */
            if (request.redirected == false) {
                response.status_code = "200";
                response.reason_phrase = "OK";
            } else {
                /* Redirect: Moved permanently */
                response.status_code = "301";
                response.reason_phrase = "Moved Permanently";
            }
            int fd;
            if ((fd = open(constructedPath.c_str(), O_RDONLY)) == -1)
                return (http_response::reject_http_response());
            char *curLine;
            while ((curLine = get_next_line(fd)))
            {
                response.payload += std::string(curLine) + "\n";
                free(curLine);
            }
            cached_resources[request.target].content = response.payload;
            cached_resources[request.target].target = request.target;
            close(fd);
        } else {
            response.status_code = "404";
            response.reason_phrase = "Not Found";
            response.header_fields["Connection"] = "close";
        }
    } else if (cached_resources[request.target].allowed_methods.count(request.method_token) == 0) { /* Not Allowed */
        response.status_code = "405";
        response.reason_phrase = "Method Not Allowed";
        response.header_fields["Connection"] = "close";
        /* RFC7231/6.5.5. must generate Allow header field */
        for (std::set<std::string>::const_iterator cit = cached_resources[request.target].allowed_methods.begin(); cit != cached_resources[request.target].allowed_methods.end(); ++cit)
            response.header_fields["Allow"] += response.header_fields.count("Allow") ? "," + *cit : *cit;
    } else if (accepted_request_methods.count(request.method_token) == 0) { /* Not Implemented */
        response.status_code = "501";
        response.reason_phrase = "Not Implemented";
        response.header_fields["Connection"] = "close";
    } else { /* OK/Redirect */
        if (request.redirected == false) {
            response.status_code = "200";
            response.reason_phrase = "OK";
        } else {
            /* Redirect: Moved permanently */
            response.status_code = "301";
            response.reason_phrase = "Moved Permanently";
        }
    }

    if (request.method_token == "DELETE" && cached_resources[request.target].allowed_methods.count("DELETE"))
    {      
        if (fileExists(cached_resources[request.target].path))
        {
            if (std::remove(cached_resources[request.target].path.c_str()) == 0)
            {
                if (request.redirected == false) {
                    response.status_code = "200";
                    response.reason_phrase = "OK";
                } else {
                    /* Redirect: Moved permanently */
                    response.status_code = "301";
                    response.reason_phrase = "Moved Permanently";
            }
            }
            else
            {
                response.status_code = "403";
                response.reason_phrase = "Forbidden";
                response.header_fields["Connection"] = "close";
            }
        }
        else
        {
            response.status_code = "404";
            response.reason_phrase = "Not Found";
            response.header_fields["Connection"] = "close";
        }
    }

    /* Control Data RFC7231/7.1. */
    /* TODO: if payload already exists, these need to check for that
        response_control_handle_age(response);
        response_control_cache_control(response);
        response_control_expires(response);
        response_control_date(response);
        response_control_location(response);
        response_control_retry_after(response);
        response_control_vary(response);
        response_control_warning(response);
    */

    representation_metadata(request, response);

    payload_header_fields(request, response);

    /* add payload */
    if (response.payload.size() == 0 && match(response.status_code, "4..") == false && cached_resources[request.target].is_static == false) /* if resource exists and is dynamic */
    {
        LOG("cgi was already handled at this point I think?");
        assert(false);
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
        CGI script(cgi_pipe, &request);
        add_script_meta_variables(script, request);
        script.execute();
        close(cgi_pipe[WRITE_END]);
        /* register an event for this socket */
        events->addReadEvent(cgi_pipe[READ_END], (int *)&cgi_responses->find(cgi_pipe[READ_END])->first);
        return (http_response::reject_http_response());
    }
    if (match(response.status_code, "[23]..") == true) {
        if (response.header_fields.count("Content-Length"))
        {
            if (request.method_token == "DELETE")
            {
                response.header_fields["Content-Length"] = std::to_string(cached_resources["/successfulDelet"].content.length());
                response.payload = cached_resources["/successfulDelet"].content;
            }
            else
            {
                if (request.header_fields.count("Range"))
                {
                    // bytes=0-100
                    if (match(request.header_fields.at("Range"), "bytes=[0-9]+-[0-9]+"))
                    {
                        int first = std::stoi(request.header_fields.at("Range").c_str() + 6);
                        int second = std::stoi(request.header_fields.at("Range").c_str() + request.header_fields.at("Range").find_first_of("-") + 1);
                        std::string payload;
                        if (response.payload.size())
                            payload = response.payload.substr(0, std::max(first, second));
                        else
                            payload = cached_resources[request.target].content.substr(0, std::max(first, second));
                        response.payload = payload;
                        response.header_fields["Content-Length"] = std::to_string(std::min(std::max(first, second), (int)payload.size()));
                    }
                }
                else
                {
                    if (response.payload.size() == 0)
                        response.payload = cached_resources[request.target].content.substr(0, std::atoi(response.header_fields["Content-Length"].c_str()));
                }
            }
        }
        else
        {
            if (response.payload.size() == 0)
                response.payload = cached_resources[request.target].content;
        }
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
    if (cached_resources.count(request.target) == 0 || response.payload.size())
    {
        response.header_fields["Content-Type"] = "text/html";
        response.header_fields["Content-Language"] = "en-US";
        return ;
    }
    response.header_fields["Content-Type"] = cached_resources[request.target].content_type;
    for (std::set<std::string>::const_iterator cit = cached_resources[request.target].content_encoding.begin(); cit != cached_resources[request.target].content_encoding.end(); ++cit)
        response.header_fields["Content-Encoding"] += response.header_fields.count("Content-Encoding") ? "," + *cit : *cit;
    for (std::set<std::string>::const_iterator cit = cached_resources[request.target].content_language.begin(); cit != cached_resources[request.target].content_language.end(); ++cit)
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
    if (match(response.status_code, "[45]..") == true)
    {
        if (response.status_code == "403")
            response.header_fields["Content-Length"] = std::to_string(cached_resources["/error403"].content.length());
        else
            response.header_fields["Content-Length"] = std::to_string(cached_resources["/error"].content.length());
        return ;
    }
    if (response.header_fields.count("Transfer-Encoding") == 0) /* set up Content-Length */
    {
        if (response.status_code == "204" || match(response.status_code, "1..") == true)
            /* No Content-Length header field */;
        else
        {
            if (response.payload.size())
                response.header_fields["Content-Length"] = std::to_string(response.payload.size());
            else
                response.header_fields["Content-Length"] = std::to_string(cached_resources[request.target].content.length());
        }
    }
    if (response.status_code == "204" || match(response.status_code, "1..") == true
        || (request.method_token == "CONNECT" && match(response.status_code, "2..") == true))
        /* No Transfer-Encoding header field */;
    else
    {
        if (response.payload.size() == 0)
            for (std::set<std::string>::const_iterator cit = cached_resources[request.target].content_encoding.begin(); cit != cached_resources[request.target].content_encoding.end(); ++cit)
                response.header_fields["Transfer-Encoding"] += response.header_fields.count("Transfer-Encoding") ? "," + *cit : *cit;
    }
}

/* responds to 'socket' based on set parameters
* 1. Construct http response
* 2. Send response
* Warning: The message might not fit into the 'send' buffer which is not handled
*/
void server::router(int socket, const http_response &response)
{
    PRINT_HERE();
    std::string message = response.http_version + " " + response.status_code + " " + response.reason_phrase + "\n";
    // LOG("\nresponse message:");
    for (std::map<std::string, std::string>::const_iterator cit = response.header_fields.begin(); cit != response.header_fields.end(); ++cit)
    {
        message += cit->first + ": " + cit->second + "\n";
        // LOG(message);
    }
    message += "\n";
    message += response.payload;
    PRINT_HERE();
    HTTP_MESSAGE_LOG("\n\n\n\nLog ID: " << http_message_log_id++ << "\n------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------"
        << std::endl << displayTimestamp() << " [RESPONSE to socket - " << socket << "]" << std::endl << message << std::endl
        << "------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    // LOG(message);
    LOG(displayTimestamp() << " RESPONSE -> [status: " << response.status_code << " - " << response.reason_phrase << "]");
    NETWORK_LOG("Log ID: " << network_log_id++ << "\n" << displayTimestamp() << " RESPONSE -> [status: " << response.status_code << " - " << response.reason_phrase << "]");
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

/* Add request meta-variables to the script RFC3875/4.1. */
void server::add_script_meta_variables(CGI &script, http_request &request)
{
    request.target = isAllowedDirectory2(request.target);
    // script.add_meta_variable("AUTH_TYPE", "");
    if (request.payload.empty() == false)
        script.add_meta_variable("CONTENT_LENGTH", std::to_string(request.payload.length()));
    if (request.header_fields.count("Content-Type"))
        script.add_meta_variable("CONTENT_TYPE", request.header_fields.at("Content-Type"));
    // script.add_meta_variable("CONTENT_TYPE", "test/file");
    // script.add_meta_variable("PATH_INFO", cached_resources[request.target].path);
    // script.add_meta_variable("PATH_INFO", request.abs_path);
    char *cwd;
    if ((cwd = getcwd(NULL, 0)) == NULL)
        TERMINATE("getcwd failed in 'add_script_meta_variables'");
    /* This should be handled from the calling server */
    script.add_meta_variable("PATH_INFO", cwd + std::string("/") + request.target);
    LOG("\nCWD");
    LOG("cwd            = " << cwd);
    LOG("request.target = " << request.target);
    LOG("");
    // script.add_meta_variable("PATH_INFO", "/Users/jludt/Desktop/Ecole42/Webserv/YoupiBanane/youpi.bla"); // needs to be changed!!!!
    // script.add_meta_variable("PATH_TRANSLATED", "/Users/jludt/Desktop/Ecole42/Webserv/YoupiBanane/youpi.bla"); // needs to be changed
    // script.add_meta_variable("PATH_TRANSLATED", cached_resources[request.target].path);
    // script.add_meta_variable("PATH_TRANSLATED", "temp/temp_cgi_file_in");
    script.add_meta_variable("PATH_TRANSLATED", cwd + std::string("/") + request.target);
    script.add_meta_variable("QUERY_STRING", request.query);
    script.add_meta_variable("REMOTE_ADDR", request.hostname);
    script.add_meta_variable("REMOTE_HOST", request.hostname);
    /* if AUTH_TYPE is set then
    * script.add_meta_variable("REQUEST_USER", "");
    */
    script.add_meta_variable("REQUEST_METHOD", request.method_token);
    if (request.extension.empty() == false)
        script.add_meta_variable("SCRIPT_NAME", std::string(cwd) + "/" + request.extension);
    else
        script.add_meta_variable("SCRIPT_NAME", "." + request.abs_path);
    script.add_meta_variable("SERVER_NAME", this->hostname);
    script.add_meta_variable("SERVER_PORT", std::to_string(this->server_port));
    script.add_meta_variable("SERVER_PROTOCOL", this->http_version);
    std::string tempHost = request.URI;
    // script.add_meta_variable("REQUEST_URI", "/Users/jludt/Desktop/Ecole42/Webserv/YoupiBanane/youpi.bla"); //needs to be changed
    script.add_meta_variable("REQUEST_URI", cwd + std::string("/") + request.target);
    LOG("ASCII");
    for (unsigned int i = 0; i < tempHost.size(); ++i)
        printf("[%d] ", tempHost[i]);
    LOG("");
    LOG("URI: " << request.URI);
    // script.add_meta_variable("REQUEST_URI", tempHost + request.target);
    // script.add_meta_variable("REQUEST_URI", "http://localhost/yo.bla");
    /* name/version of the server, no clue what this means currently.. */
    // script.add_meta_variable("SERVER_SOFTWARE", "");
    for (std::map<std::string, std::string>::iterator it = request.header_fields.begin(); it != request.header_fields.end(); ++it)
    {
        if (it->first == "Authorization" || it->first == "Content-Length" || it->first == "Content-Type"
            || it->first == "Connection")
            continue ;
        std::string key = "HTTP_" + to_upper(it->first);
        std::replace(key.begin(), key.end(), '-', '_');
        script.add_meta_variable(key, it->second);
    }
}

std::string	server::displayTimestamp(void)
{
	time_t		       now = time(0);
    std::ostringstream buffer;

    tm *ltm = localtime(&now);
    buffer << "[";
    if (ltm->tm_hour < 10)
        buffer << '0';
    buffer << ltm->tm_hour << ":";
    if (ltm->tm_min < 10)
        buffer << '0';
    buffer << ltm->tm_min << ":";
    if (ltm->tm_sec < 10)
        buffer << '0';
	buffer << ltm->tm_sec << "] ";
    return buffer.str();
}

int server::fileExists(const std::string& file)
{
    struct stat buf;
    if (stat(file.c_str(), &buf) == 0)
    {
        if (buf.st_mode & S_IFDIR) /* directory */
            return (2);
        else if (buf.st_mode & S_IFREG) /* file */
            return (1);
        return (0);
    }
    return (0);
}

http_response server::handle_post_request(http_request &request)
{
    /* RFC7231/4.3.3.
    * CGI for example comes here
    * if one or more resources has been created, status code needs to be 201 with "Created"
    *   with "Location" header field that provides an identifier for the primary resource created
    *   and a representation that describes the status of the request while referring to the new resource(s).
    * If no resources are created, respond with 200 that containts the result and a "Content-Location"
    *   header field that has the same value as the POST's effective request URI
    * If result is equivalent to already existing resource, redirect with 303 with "Location" header field
    */
    size_t pos = request.target.find_last_of('.');
    std::string extension;
    if (pos != std::string::npos)
        extension = request.target.substr(pos);
    request.extension = server_configuration.general_cgi_path;
    request.underLocation = isAllowedDirectory2(request.target);
    LOG("request.underLocation: " << request.underLocation);
    if (server_configuration.general_cgi_extension.size() && extension == server_configuration.general_cgi_extension)
    {   /*
         * if general_cgi_extension exists in config file and the request resource matches this extension
         * -> general_cgi_path should exist, run it
         */
        int cgi_pipe[2];
        if (pipe(cgi_pipe) == -1)
            TERMINATE("pipe failed");
        // if (fcntl(cgi_pipe[READ_END], F_SETFL, O_NONBLOCK) == -1)
        //     TERMINATE("fcntl failed");
        // if (fcntl(cgi_pipe[WRITE_END], F_SETFL, O_NONBLOCK) == -1)
        //     TERMINATE("fcntl failed");
        /* write request payload into socket */
        (*cgi_responses)[cgi_pipe[READ_END]] = request.socket; /* to serve client later */
        CGI script(cgi_pipe, &request);
        add_script_meta_variables(script, request);
        script.execute();
        close(cgi_pipe[WRITE_END]);
        /* register an event for this socket */
        int *a = (int *)malloc(sizeof(int));
        *a = cgi_pipe[READ_END];
        events->addReadEvent(cgi_pipe[READ_END], a);
        PRINT_HERE();
        return (http_response::cgi_response());
    }
    if (request.underLocation.size()) /* location exists */
    {
        std::string location;
        for (std::map<std::string, t_location>::const_reverse_iterator cit = sortedRoutes.rbegin(); cit != sortedRoutes.rend(); ++cit)
        {
            if (request.target.substr(0, cit->first.size()) == cit->first)
            {
                location = cit->first;
                break ;
            }
        }
        assert(location.size()); /* location should exist */
        LOG("location: " << location);
        if (sortedRoutes[location].cgi_extension.size() && sortedRoutes[location].cgi_extension == extension)
        {
            /*
            * if cgi_extension exists in location and the request resource matches this extension
            * -> cgi_path should exist, run it
            */
            int cgi_pipe[2];
            if (pipe(cgi_pipe) == -1)
                TERMINATE("pipe failed");
            // if (fcntl(cgi_pipe[READ_END], F_SETFL, O_NONBLOCK) == -1)
            //     TERMINATE("fcntl failed");
            // if (fcntl(cgi_pipe[WRITE_END], F_SETFL, O_NONBLOCK) == -1)
            //     TERMINATE("fcntl failed");
            /* write request payload into socket */
            (*cgi_responses)[cgi_pipe[READ_END]] = request.socket; /* to serve client later */
            CGI script(cgi_pipe, &request);
            add_script_meta_variables(script, request);
            script.execute();
            close(cgi_pipe[WRITE_END]);
            /* register an event for this socket */
            int *a = (int *)malloc(sizeof(int));
            *a = cgi_pipe[READ_END];
            events->addReadEvent(cgi_pipe[READ_END], a);
            return (http_response::cgi_response());
        }
        for (std::vector<std::string>::iterator it = sortedRoutes[location].methods.begin();
            it != sortedRoutes[location].methods.end(); ++it)
        {
            LOG("*it: " << *it);
            if (*it == "PUT" && request.method_token == "PUT")
            { /* check if files exists -> if so update it */
                std::ofstream uploaded_file(request.underLocation);
                PRINT_HERE();
                LOG("request.underLocation: " << request.underLocation);
                if (!uploaded_file)
                    WARN("Failed to create file: " + request.underLocation);
                // LOG(request.payload);
                uploaded_file << request.payload;
                // LOG("request.payload: " << request.payload);
                http_response response;
                response.http_version = "HTTP/1.1";
                if (request.redirected == false) {
                    response.status_code = "200";
                    response.reason_phrase = "OK";
                } else {
                    /* Redirect: Moved permanently */
                    response.status_code = "301";
                    response.reason_phrase = "Moved Permanently";
                }
                response.header_fields["Content-Type"] = "text/html";
                // response.header_fields["Content-Length"] = std::to_string(request.payload.size());
                // response.payload = request.payload;
                response.header_fields["Connection"] = "close";
                response.header_fields["Content-Length"] = std::to_string(0);
                return (response);
            }
            else if (*it == "POST" && request.method_token == "POST")
            { /* creates and overwrites resource */
                std::ofstream uploaded_file(request.underLocation);
                PRINT_HERE();
                LOG("uploaded_file: " << uploaded_file);
                if (!uploaded_file)
                    WARN("Failed to create file: " + request.underLocation);
                uploaded_file << request.payload;
                // LOG("request.payload: " << request.payload);
                http_response response;
                response.http_version = "HTTP/1.1";
                if (request.redirected == false) {
                    response.status_code = "200";
                    response.reason_phrase = "OK";
                } else {
                    /* Redirect: Moved permanently */
                    response.status_code = "301";
                    response.reason_phrase = "Moved Permanently";
                }
                response.header_fields["Content-Type"] = "text/html";
                // response.header_fields["Content-Length"] = std::to_string(request.payload.size());
                // response.payload = request.payload;
                response.header_fields["Connection"] = "close";
                response.header_fields["Content-Length"] = std::to_string(0);
                return (response);
            }
        }
        /* Respond with method not allowed */
        return (http_response::reject_http_response());
    }
    /* 404 not found */
    http_response response;
    response.http_version = "HTTP/1.1";
    response.status_code = "404";
    response.reason_phrase = "Not Found";
    response.header_fields["Connection"] = "close";
    response.header_fields["Content-Type"] = "text/html";
    response.header_fields["Content-Length"] = std::to_string(cached_resources["/error"].content.length());
    response.payload = cached_resources["/error"].content;
    return (response);
}

std::string     server::decoding_chunked(const std::string &chunked)
{
    std::string         unchunked = "";
    std::string         hexdec_size;
    int                 chunked_size;
    std::stringstream   ss;
    size_t              i = 0;
    
    while (true)
    {
        // get chunked-hexdecimal size
        for (; chunked[i] != '\r' && chunked[i] != ' '; ++i)
            hexdec_size += chunked[i];
        // transform chunk-size from hexdezimal to decimal
        ss << std::hex << hexdec_size;
        ss >> chunked_size;
        ss.clear();
        chunks_size += chunked_size;
        if (chunked_size < 1)
            break;
        // getting to body;
        while (chunked[i] != '\n')
            i++;
        // read chunked body
        for (int j = 0; j < chunked_size; ++j)
            unchunked += chunked[++i];
        // getting to chunked_size()
        while (chunked[i] != '\n')
            i++;
        i++;
        hexdec_size.clear();
    }
    return unchunked;
}
