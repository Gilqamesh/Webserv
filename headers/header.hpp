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
# include <poll.h>          // poll
# include <sys/select.h>    // select
# include <pthread.h>       // <thread> is c++11
# include <csignal>         // to close server socket if process is signalled

# define LOG(x) (std::cout << x << std::endl)
# define LOG_E(x) (std::cerr << x << std::endl)
# define LOG_TIME(x) (std::cout << x << " time: " << (get_current_timestamp() - start_timestamp) / 1000000.0 << " seconds" << std::endl)
# define WARN(x) (LOG_E("Warning: " << x))
# define TERMINATE(x) ({\
    perror(x);\
    exit(EXIT_FAILURE);\
})
# define PRINT_HERE() (LOG(__FILE__ << " " << __LINE__))
# define TIMEOUT_TO_CUT_CONNECTION 5 /* in seconds */
# define CRLF std::string("\x0d") /* Chrome CRLF is CR */

#endif
