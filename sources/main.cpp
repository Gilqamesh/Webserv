#include "server.hpp"

int main(void)
{
    server test_server(40, 3);
    test_server.cache_file("views/index.html", "/");
    test_server.cache_file("views/about.html", "/about");
    test_server.cache_file("views/error.html", "/error");
    test_server.cache_file("test", "/test", false);
    test_server.server_listen(); /* main loop of server */
}
