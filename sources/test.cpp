// #include "server.hpp"
// #include "utils.hpp"
// #include <errno.h>
// #include <unistd.h>

// #define BUFFER_SIZE 10000

// int main(int argc, char **argv)
// {
//     (void)argc;
//     (void)argv;
//     /* TEST match_pattern */
//     // LOG(match_pattern("hello", "hello"));
//     // LOG(match_pattern("llll", "[l]*"));
//     // LOG(match_pattern("llll", "[l]"));
//     // LOG(match_pattern("aaa aa aaaaaa", "[a]+ [a]+ [a]+"));
//     // LOG(match_pattern("a a", "[a] [a]"));
//     // LOG(match_pattern("a a", "a a"));
//     // LOG(match_pattern("aaa aa aaaaaa" + CRLF, "[a]+ [a]+ [a]+" + CRLF));
//     // LOG(match_pattern("aaa b caaabaa", "[abc]+ [abc]+ [abc]+"));
//     // LOG(match_pattern("hello", "..l.o"));
//     // LOG(match_pattern("hello", "c.l.o"));
//     // LOG(match_pattern("aaa aa aaaaa", "a*.[a]+ [^ ]*"));
//     // LOG(match_pattern("[hi", "\\[h\\."));
//     // LOG(match_pattern("[h.", "\\[h\\."));
//     // LOG(match_pattern("abc", "[a-d]"));
//     // LOG(match_pattern("abc", "[a-d]"));
//     // LOG(match_pattern("abc", "[^a-d]"));
//     // LOG(match_pattern("abc", "[^d-e]*"));
//     // LOG(match_pattern("abc", "[^b]*.c"));

//     // LOG(match_pattern("GET / HTTP/1.1", "[^ ]* [^ ]* [^ ]*"));
//     // LOG(match_pattern("GET / HTTP/1.1\n", ".*"));
//     // LOG(match_pattern("GET / HTTP/1.1\n", ".*\n"));
//     // LOG(match_pattern("Host: 10.12.1.1:8080\n", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
//     // LOG(match_pattern("Host: 10.12.1.1:8080", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
//     // LOG(match_pattern("Host: 10.12.1.1:8080", "[^ \t:]*:[ \t]*.*[ \t]*"));
//     // LOG(match_pattern("Host: 10.12.1.1:8080" + CRLF, "[^ \t:]*:[ \t]*[ -~]*[ \t]*" + CRLF));
//     // LOG(match_pattern("Connection: keep-alive", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
//     // LOG(match_pattern("Connection: keep", "[^ \t:]*:[ \t]*[ -~]*"));

//     /* TEST sending messages to server */
//     // int sock = 0;
//     // struct sockaddr_in serv_addr;
//     // std::string hello = "GET / HTTP/1.1" + CRLF;
//     // char buffer[1024];
//     // buffer[1023] = '\0';
//     // if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
//     // {
//     //     printf("\n Socket creation error \n");
//     //     return -1;
//     // }
    
//     // // if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
//     // //     TERMINATE("fcntl failed");

//     // memset(&serv_addr, '0', sizeof(serv_addr));
//     // serv_addr.sin_family = AF_INET;
//     // serv_addr.sin_port = htons(40);
    
//     // // Convert IPv4 and IPv6 addresses from text to binary form
//     // std::string IP = "127.0.0.1";
//     // if (inet_pton(AF_INET, IP.c_str(), &serv_addr.sin_addr) <= 0)
//     // {
//     //     printf("\nInvalid address/ Address not supported \n");
//     //     return -1;
//     // }

//     // if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
//     //     TERMINATE(("Connection Failed " + std::to_string(sock)).c_str());

//     // if (send(sock , hello.c_str(), hello.length(), 0 ) == -1)
//     //     TERMINATE("send failed");

//     // printf("Hello message sent\n");
//     // if (recv(sock, buffer, 1024, 0) == -1)
//     //     TERMINATE("recv failed");

//     // printf("%s\n",buffer );
//     // return 0;

//     // TEST getcwd
//     // LOG(getcwd(NULL, 0));

//     // TEST to_upper
//     // LOG(to_upper("a8c&sLSdoA_SD2dfc ;a"));

//     // TEST replace
//     // std::string str("-asd_-daslads924-3-9fds8_");
//     // std::replace(str.begin(), str.end(), '-', '_');
//     // LOG(str);

//     // TEST CGI script
//     // if (argc != 2)
//     //     TERMINATE("usage: ./<app_name> <file_to_serve>");
//     // (void)argc;
//     // (void)argv;
//     // char buffer[BUFFER_SIZE];
//     // memset(buffer, 0, BUFFER_SIZE);
//     // int fd;
//     // if ((fd = open("index.html", O_RDONLY)) == -1)
//     //     TERMINATE("open failed in 'test' script");
//     // read(fd, buffer, BUFFER_SIZE - 1);
//     // close(fd);
//     // std::string response(buffer);
//     // write(STDOUT_FILENO, response.data(), response.length());
//     // LOG_E("script has finished executing");

//     // (void)argc;
//     // int fd;
//     // if ((fd = open(argv[1], O_RDONLY)) == -1)
//     //     return (-1);
//     // std::string tmp;
//     // std::string end;
//     // while ((tmp = get_next_line(fd)).size())
//     //     end += tmp;
//     // LOG(end);
//     // (void)argc;
//     // (void)argv;
//     // LOG(match_pattern("http /directory/ TTP/1.1" + CRLF, HEADER_RESPONSE_LINE_PATTERN));
//     // LOG(match_pattern("http /directory/ HTTP/1.1" + CRLF, HEADER_RESPONSE_LINE_PATTERN));
//     int fd;
//     fd = open(argv[1], O_RDONLY);
//     char *line;
//     while ((line = get_next_line(fd)))
//     {
//         printf("%s", line);
//         free(line);
//     }
// }
