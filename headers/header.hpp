#ifndef HEADER_HPP
# define HEADER_HPP

# include <iostream>
# include <cstring>
# include <sys/socket.h> // socket, listen, connect, bind
# include <netinet/in.h> // struct sockaddr_in
# include <arpa/inet.h>  // long/short host/network conversion: htons, htonl, ntohl, ntohs
                         // inet_addr
# include <unistd.h>     // write, read, close
# include <string>
# include <fstream>
# include <sstream>
# include <dirent.h> // opendir
# include <regex.h> // 

# include <poll.h>          // poll
# include <sys/select.h>    // select
# include <pthread.h>       // <thread> is c++11
# include <csignal>         // to close server socket if process is signalled
# include <fcntl.h>         // fcntl

# include <sys/types.h> // kqueue, kevent
# include <sys/event.h>
# include <sys/time.h>

# define LOG(x) (std::cout << x << std::endl)
# define LOG_E(x) (std::cerr << x << std::endl)
# define LOG_TIME(x) (std::cout << x << " time: " << (get_current_timestamp() - start_timestamp) / 1000000.0 << " seconds" << std::endl)
# define WARN(x) (LOG_E("Warning: " << x))
# define PRINT_HERE() (LOG(__FILE__ << " " << __LINE__))
# define TERMINATE(x) do {  \
    PRINT_HERE();           \
    perror(x);              \
    exit(EXIT_FAILURE);     \
    } while (0)
# define TIMEOUT_TO_CUT_CONNECTION 5 /* in seconds */
# define CRLF std::string("\x0d") /* Chrome CRLF is CR */

extern std::ofstream network_log;
# define NETWORK_LOG(x) (network_log << x << std::endl)

extern std::ofstream http_message_log;
# define HTTP_MESSAGE_LOG(x) (http_message_log << x << std::endl)

/* for pipes */
# define READ_END   0
# define WRITE_END  1

#endif
