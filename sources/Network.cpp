#include "Network.hpp"
#include "http_response.hpp"

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
	    serverNetwork[i].construct(configs[i].port, BACKLOG, start_timestamp_network, &cgi_responses, &events, configs[i]);
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
                    char *curLine;
                    while ((curLine = get_next_line(*(int *)events[i].udata)))
                    {
                        response += std::string(curLine);
                        free(curLine);
                    }
                    send(cgi_responses[*(int *)events[i].udata], response.data(), response.length(), 0);
                    LOG("CGI Response:\n" << response);
                    response.clear();
                    int cgi_out = open("temp/temp_cgi_file_out", O_RDONLY);
                    if (cgi_out == -1)
                        WARN("open failed for reading: temp/temp_cgi_file_out");
                    char buffer[4096];
                    while (1)
                    {
                        int readRet = read(cgi_out, buffer, 4096);
                        if (readRet == -1)
                        {
                            PRINT_HERE();
                            WARN("read failed");
                            break ;
                        }
                        if (readRet == 0)
                            break ;
                        send(cgi_responses[*(int *)events[i].udata], buffer, readRet, 0);
                    }
                    close(cgi_out);
                    /* cut connection with the client */
                    usleep(10000);
                    servers[sockets[cgi_responses[*(int *)events[i].udata]]].cut_connection(cgi_responses[*(int *)events[i].udata]);
                }
                PRINT_HERE();
                /* remove client socket from 'sockets' */
                sockets.erase(cgi_responses[*(int *)events[i].udata]);
                /*  */
                cgi_responses.erase(*(int *)events[i].udata);
                close(*(int *)events[i].udata);
                free(events[i].udata);
                events[i].udata = NULL;
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
                    PRINT_HERE();
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
                PRINT_HERE();
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
