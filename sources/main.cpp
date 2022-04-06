#include "Network.hpp"

std::ofstream network_log("logs/network_log");
int network_log_id = 0;
std::ofstream http_message_log("logs/http_message_log");
int http_message_log_id = 0;

int main(int argc, char **argv)
{
    Network                 network;

    if (argc != 2)
        return (1);

    network.initNetwork(argv[1]);
    network.runNetwork();

    // test_server.cache_file("views/index.html", "/");
    // test_server.cache_file("views/about.html", "/about");
    // test_server.cache_file("views/error.html", "/error");
    // test_server.cache_file("test", "/test", false);
    // test_server.cache_file("cgi_tester", "/cgi_tester", false);
}
