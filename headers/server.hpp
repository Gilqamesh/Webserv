#ifndef SERVER_HPP
# define SERVER_HPP

# include "header.hpp"
# include "Network.hpp"
# include <set>
# include <unordered_set>
# include <map>
# include <unordered_map>
# include <vector>
# include "http_request.hpp"
# include "http_response.hpp"
# include "resource.hpp"

# include <sys/types.h> // kqueue, kevent, EV_SET
# include <sys/event.h>
# include <sys/time.h>

# define MAX_EVENTS 32 

# define HEADER_WHITESPACES " \t"
# define HEADER_FIELD_PATTERN "[^ \t:]*:[ \t]*[ -~]*[ \t]*" + CRLF
# define HEADER_REQUEST_LINE_PATTERN "[!-~]+ [!-~]+ HTTP/[1-3][.]*[0-9]*" + CRLF /* match_pattern needs support on '?' and '()' (capture group) for proper parsing */
# define HEADER_RESPONSE_LINE_PATTERN "[!-~]+ [!-~]+ [!-~]*" + CRLF


class server
{
private:
    int                                             server_socket_fd;
    int                                             server_port;
    int                                             server_backlog; /* server_socket_fd - backlog */
	std::set<int>                                   connected_sockets_set; /* socket */
    std::unordered_map<std::string, resource>       cached_resources; /* route - resource */
    /* constants */
    std::unordered_set<std::string>                 accepted_request_methods;
    std::unordered_set<char>                        header_whitespace_characters;
    std::string                                     http_version;
    unsigned long                                   start_timestamp;
public:
    server();
    ~server();

    void            construct(int port, int backlog, unsigned long timestamp);
    void 			server_listen(void);
    void 			cache_file(const std::string &path, const std::string &route);
    int const       &getServerSocketFd(void) const;
    int             accept_connection(int kq, int socket, struct kevent *event);
    void            cut_connection(int kq, int socket, struct kevent *event);
    void            handle_connection(int kq, int socket, struct kevent *event);
    void            send_timeout(int socket); /* Send response: 408 Request Timeout */

    server(const server& other);
    server &operator=(const server& other);
private:

    http_request    parse_request_header(int socket);
    void            router(int socket, const http_response &response);

    /* format http request and its control functions */
    void            format_http_request(http_request& request);
    void            request_control_cache_control(http_request &request);
    void            request_control_expect(http_request &request);
    void            request_control_host(http_request &request);
    void            request_control_max_forwards(http_request &request);
    void            request_control_pragma(http_request &request);
    void            request_control_range(http_request &request);
    void            request_control_TE(http_request &request);

    /* format http response and its control functions */
    http_response   format_http_response(const http_request& request);
    void            response_control_handle_age(http_response &response);
    void            response_control_cache_control(http_response &response);
    void            response_control_expires(http_response &response);
    void            response_control_date(http_response &response);
    void            response_control_location(http_response &response);
    void            response_control_retry_after(http_response &response);
    void            response_control_vary(http_response &response);
    void            response_control_warning(http_response &response);

    void            representation_metadata(const http_request &request, http_response &response); /* Header field config for payload RFC7231/3.1. */
    void            representation_metadata(http_request &request); /* should be the same as for response */
    void            payload_header_fields(const http_request &request, http_response &response);

    void            initialize_constants(void); // helper
};

#endif
