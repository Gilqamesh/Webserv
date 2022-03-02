#include "Network.hpp"

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
