#include "header.hpp"

// restrict is not a keyword in C++
// #define __restrict
int accept_connection(int socket, struct sockaddr * /* restrict */ address,
socklen_t * /* restrict */address_len)
{
    return (accept(socket, address, address_len));
}

int main(void)
{
    std::string index;
    std::ifstream ifs("sources/index.html");
    if (!ifs)
        TERMINATE("could not open index.html");
    std::string tmp;
    while (std::getline(ifs, tmp))
        index += tmp;

    /* creating a socket (domain/address family, type of service, specific protocol)
    * AF_INET       -   IP address family
    * SOCK_STREAM   -   virtual circuit service
    * 0             -   only 1 protocol exists for this service
    */
    int	server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
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
    address.sin_port = htons(PORT);
    if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == -1)
        TERMINATE("failed to bind the socket");

    /* wait/listen for connection
    * listen(socket, backlog)
    *   the backlog defines the maximum number of pending connections that can be queued up
    *   before connections are refused
    * accept(socket, address, socket length) - grabs the first connection request on the queue
    *   of pending connections and creates a new socket for that connection
    *   The original socket that is set up for listening is used only for accepting connections,
    *   not for exchanging data.
    */
    if (listen(server_fd, MAX_NUMBER_OF_CONNECTIONS) == -1)
        TERMINATE("listen failed");
    socklen_t address_length;
    int new_socket = accept_connection(server_fd, reinterpret_cast<struct sockaddr *>(&address), &address_length);
    if (new_socket == -1)
        TERMINATE("accept failed");

    LOG("Client joined from port: " << address.sin_port);


    /* send and receive messages using read and write to the socket */
    while (1)
    {
        std::string header("HTTP/1.1 200 OK\nContent-Type: text/html;charset=UTF-8\nContent-Length: " + std::to_string(index.length()) + "\n\n" + index);
        write(new_socket, header.c_str(), header.length());
        break ;
    }

    /* close socket after we are done communicating*/
    close(new_socket);

    /* close server socket */
    close(server_fd);
}
