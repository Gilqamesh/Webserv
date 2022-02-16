#include "server.hpp"
#include "conf_file.hpp"

int main(int argc, char **argv)
{
    pid_t               pid;
    conf_file           configurations(argv[1]);
    std::vector<t_server> configs;

    configs = configurations.get_configs();

    if (argc != 2)
        return (1);

    for  (size_t i = 0; i < configs.size(); i++)
    {
        pid = fork();
        if (pid < 0)
            exit(1);
        else if (pid == 0)
        {
            server  webserv(configs[i].port, 3);
            for (size_t j = 0; j < configs[i].locations.size(); j++)
                webserv.cache_file(configs[i].locations[j].root + "/" + configs[i].locations[j].index, configs[i].locations[j].path_name);
            webserv.server_listen();
        }
    }
    wait(NULL);

    return (0);
}
