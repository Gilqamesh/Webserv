#include "Network.hpp"

std::ofstream network_log("logs/network_log");
int network_log_id = 0;
std::ofstream http_message_log("logs/http_message_log");
int http_message_log_id = 0;

int main(int argc, char **argv)
{
    Network network;

    if (argc != 2)
        TERMINATE("Usage: ./webserv <config_file_path>");

    try
    {
        network.initNetwork(argv[1]);
        network.runNetwork();
    }
    catch (std::exception &e)
    {
        LOG(e.what());
    }
}
