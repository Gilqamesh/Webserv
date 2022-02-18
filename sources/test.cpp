#include "server.hpp"
#include "utils.hpp"
#include <errno.h>

int main(void)
{
    /* TEST match_pattern */
    // LOG(match_pattern("hello", "hello"));
    // LOG(match_pattern("llll", "[l]*"));
    // LOG(match_pattern("llll", "[l]"));
    // LOG(match_pattern("aaa aa aaaaaa", "[a]+ [a]+ [a]+"));
    // LOG(match_pattern("a a", "[a] [a]"));
    // LOG(match_pattern("a a", "a a"));
    // LOG(match_pattern("aaa aa aaaaaa" + CRLF, "[a]+ [a]+ [a]+" + CRLF));
    // LOG(match_pattern("aaa b caaabaa", "[abc]+ [abc]+ [abc]+"));
    // LOG(match_pattern("hello", "..l.o"));
    // LOG(match_pattern("hello", "c.l.o"));
    // LOG(match_pattern("aaa aa aaaaa", "a*.[a]+ [^ ]*"));
    // LOG(match_pattern("[hi", "\\[h\\."));
    // LOG(match_pattern("[h.", "\\[h\\."));
    // LOG(match_pattern("abc", "[a-d]"));
    // LOG(match_pattern("abc", "[a-d]"));
    // LOG(match_pattern("abc", "[^a-d]"));
    // LOG(match_pattern("abc", "[^d-e]*"));
    // LOG(match_pattern("abc", "[^b]*.c"));

    // LOG(match_pattern("GET / HTTP/1.1", "[^ ]* [^ ]* [^ ]*"));
    // LOG(match_pattern("GET / HTTP/1.1\n", ".*"));
    // LOG(match_pattern("GET / HTTP/1.1\n", ".*\n"));
    // LOG(match_pattern("Host: 10.12.1.1:8080\n", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
    // LOG(match_pattern("Host: 10.12.1.1:8080", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
    // LOG(match_pattern("Host: 10.12.1.1:8080", "[^ \t:]*:[ \t]*.*[ \t]*"));
    // LOG(match_pattern("Host: 10.12.1.1:8080" + CRLF, "[^ \t:]*:[ \t]*[ -~]*[ \t]*" + CRLF));
    // LOG(match_pattern("Connection: keep-alive", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
    // LOG(match_pattern("Connection: keep", "[^ \t:]*:[ \t]*[ -~]*"));

    /* TEST sending messages to server */
    // int sock = 0;
    // struct sockaddr_in serv_addr;
    // std::string hello = "GET / HTTP/1.1" + CRLF;
    // char buffer[1024];
    // buffer[1023] = '\0';
    // if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    // {
    //     printf("\n Socket creation error \n");
    //     return -1;
    // }
    
    // // if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
    // //     TERMINATE("fcntl failed");

    // memset(&serv_addr, '0', sizeof(serv_addr));
    // serv_addr.sin_family = AF_INET;
    // serv_addr.sin_port = htons(40);
    
    // // Convert IPv4 and IPv6 addresses from text to binary form
    // std::string IP = "127.0.0.1";
    // if (inet_pton(AF_INET, IP.c_str(), &serv_addr.sin_addr) <= 0)
    // {
    //     printf("\nInvalid address/ Address not supported \n");
    //     return -1;
    // }

    // if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    //     TERMINATE(("Connection Failed " + std::to_string(sock)).c_str());

    // if (send(sock , hello.c_str(), hello.length(), 0 ) == -1)
    //     TERMINATE("send failed");

    // printf("Hello message sent\n");
    // if (recv(sock, buffer, 1024, 0) == -1)
    //     TERMINATE("recv failed");

    // printf("%s\n",buffer );
    // return 0;
}
