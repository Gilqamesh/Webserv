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
# include "CGI.hpp"
# include "conf_file.hpp"
# include "HttpObject.hpp"

# include <sys/types.h> // kqueue, kevent, EV_SET
# include <sys/event.h>
# include <sys/time.h>

#include <sys/stat.h> // check if file exists

#include <ctime> // to get current time

# define HEADER_WHITESPACES " \t"
# define HEADER_FIELD_PATTERN "[^ \t:]*:[ \t]*[ -~]*[ \t]*"
# define HEADER_REQUEST_LINE_PATTERN "[!-~]+ [!-~]+ HTTP/[1-3][.]*[0-9]*" /* match_pattern needs support on '?' and '()' (capture group) for proper parsing */
# define HEADER_RESPONSE_LINE_PATTERN "[!-~]+ [!-~]+ [!-~]*"

class server
{
private:
    int                                 server_socket_fd;
    int                                 server_port;
    int                                 server_backlog; /* server_socket_fd - backlog */
    std::map<std::string, resource>     cached_resources; /* route - resource */
    std::map<int, int>                  *cgi_responses; /* cgi socket - client socket */
    /* constants */ 
    std::set<std::string>               accepted_request_methods;
    std::set<char>                      header_whitespace_characters;
    std::string                         http_version;
    unsigned long                       start_timestamp;
    std::string                         hostname; /* ipv4 */
    int                                 current_number_of_connections;
    EventHandler                        *events;
    t_server                            server_configuration;
    /*
     * created when we start reading from a socket 
     * destroyed when we cut connection with the socket
     */
    std::map<int, HttpObject *>         currentHttpObjects; /* cgi socket - HttpObject begin processed */
public:
    std::map<int, std::string>          cgi_outfiles; /* cgi socket - filename */

    void            read_request(int fd);
    void            get_header_fields(int socket);
    bool            get_header_infos(int socket);
    void            get_body(int socket);
    bool            check_first_line(std::string, http_request&);
    bool            check_prebody(std::string, http_request&);

    void            construct(int port, int backlog, unsigned long timestamp, std::map<int, int> *cgiResponses, EventHandler *events,
                                const t_server &configuration);
    void 			cache_file(void);
    void            add_resource(const resource &resource);
    inline int      getServerSocketFd(void) const { return (server_socket_fd); }
    int             accept_connection(void);
    void            cut_connection(int socket);
    void            handle_connection(int socket);
    void            send_timeout(int socket); /* Send response: 408 Request Timeout */
    void            deleteHttpObject(int socket);

private:
    http_request    *parse_request_header(int socket);
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
    http_response   format_http_response(http_request& request);
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
    void            add_script_meta_variables(CGI &script, http_request &request);

    std::string     displayTimestamp(void);
    int             fileExists(const std::string& file);
    std::string     isAllowedDirectory(const std::string &target);
    std::string     isAllowedDirectory2(const std::string &target);
    std::string     isAllowedDirectory3(const std::string &target);

    http_response   handle_post_request(http_request &request);
    std::pair<std::string *, bool>     decoding_chunked(const std::string &chunked, int socket);
    void            parse_URI(http_request &request);

    std::vector<t_location> locations; /* a copy of 'locations' coming from the configuration file */
    std::map<std::string, t_location>   sortedRoutes; /* for isAllowedDirectory to handle more specific routes first instead of the generic ones */
};

#endif
