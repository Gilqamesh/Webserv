#include "header.hpp"

int main(void)
{
    /* creating a socket (domain/address family, type of service, specific protocol)
    * AF_INET       -   IP address family
    * SOCK_STREAM   -   virtual circuit service
    * 0             -   only 1 protocol exists for this service
    */
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
        TERMINATE("failed to create a socket");

    /* inet_addr(const char *cp)
    * convert the Internet host address (cp) from IPv4 numbers-and-dots
    * notation into binary data in network byte order
    * -1 is a valid return (255.255.255.255)
    */
    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("10.12.1.1");

    /* connect(sockfd, addr, addrlen)
    * connects the socket to the address
    */
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        TERMINATE("Connection failed");

    /* send a message to the server */
    std::string hello("Hello from client");
    send(client_fd, hello.c_str(), hello.length(), 0);

    /* close client socket */
    close(client_fd);
}
