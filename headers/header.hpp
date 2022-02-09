#ifndef HEADER_HPP
# define HEADER_HPP

# include <iostream>
# include <cstring>
# include <sys/socket.h> // socket, listen, connect
# include <netinet/in.h> // struct sockaddr_in
# include <arpa/inet.h>  // long/short host/network conversion: htons, htonl, ntohl, ntohs
                         // inet_addr
# include <unistd.h>     // write, read, close
# include <string>
# include <fstream>
# include <sstream>

# define LOG(x) (std::cout << x << std::endl)
# define LOG_E(x) (std::cerr << x << std::endl)
# define TERMINATE(x) ({\
    LOG_E(x);\
    exit(EXIT_FAILURE);\
})
# define PRINT_HERE() (LOG(__FILE__ << " " << __LINE__))
# define PORT 8080
# define MAX_NUMBER_OF_CONNECTIONS 3

#endif
