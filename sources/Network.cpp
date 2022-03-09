#include "Network.hpp"

void Network::initNetwork(char *file_name)
{
	// the kqueue holds all the events we are interested in
    // to start, we simply create an empty kqueue
    conf_file               config_file(file_name);
    std::vector<t_server>   configs = config_file.get_configs();
    server                  serverNetwork[configs.size()];

	start_timestamp_network = get_current_timestamp();

    for (size_t i = 0; i < configs.size(); i++)
    {
	    serverNetwork[i].construct(configs[i].port, 10, start_timestamp_network, &cgi_responses, &events);
        serverNetwork[i].cache_file(configs[i] /* configs[i].locations[j].root + "/" + configs[i].locations[j].index, configs[i].locations[j].route, false */);
	    servers.insert(std::pair<int, server>(serverNetwork[i].getServerSocketFd(), serverNetwork[i]));
    }
}

/*
* main loop of the servers
* waits for new connections or handles already established connections
*/

void Network::runNetwork()
{
	for (std::map<int, server>::iterator it = servers.begin(); it != servers.end(); ++it)
        events.addReadEvent(it->first);

    while (true)
    {
        int nev = events.getNumberOfEvents();

        for (int i = 0; i < nev; ++i)
        {
            int fd = events[i].ident;
            if (events[i].udata != NULL) /* CGI fd */
            {
                assert(cgi_responses.count(*(int *)events[i].udata) != 0);
                if (sockets.count(cgi_responses[*(int *)events[i].udata])) /* if we still have connection with the client send the response */
                {
                    std::string response;
                    std::string tmp;
                    while ((tmp = get_next_line(*(int *)events[i].udata)).length())
                        response += tmp;
                    LOG("CGI Response: " << response);
                    send(cgi_responses[*(int *)events[i].udata], response.data(), response.length(), 0);
                }
                /* cut connection with the client */
                servers[sockets[cgi_responses[*(int *)events[i].udata]]].cut_connection(cgi_responses[*(int *)events[i].udata]);
                /* remove client socket from 'sockets' */
                sockets.erase(cgi_responses[*(int *)events[i].udata]);
                /*  */
                cgi_responses.erase(*(int *)events[i].udata);
                close(*(int *)events[i].udata);
                continue ;
            }
            if (servers.count(fd)) /* if 'fd' is a server socket -> accept connection */
            {
                int new_socket;
                /* if could not accept connection continue to the next event */
                if ((new_socket = servers[fd].accept_connection()) == -1)
                    continue ;
                /* otherwise map the socket to the server socket */
                sockets[new_socket] = fd;
            }
			else if (events[i].filter == EVFILT_READ)   /* socket is ready to be read */
            {
                if (events[i].flags & EV_EOF) /* client side shutdown */
                {
                    servers[sockets[fd]].cut_connection(fd);
                    sockets.erase(fd);
                    continue ;
                }
                /* update socket's timeout event */
                events.addTimeEvent(fd, TIMEOUT_TO_CUT_CONNECTION * 1000);
                /* handle connection */
            	servers[sockets[fd]].handle_connection(fd);
            }
            else if (events[i].filter == EVFILT_TIMER)  /* socket is expired */
            {
				servers[sockets[fd]].send_timeout(fd); /* 408 Request Timeout */
                servers[sockets[fd]].cut_connection(fd);
                sockets.erase(fd);
            }
        }
    }
}

Network::Network() {}

Network::~Network()
{
    // ToDo: cut connection of servers
}
