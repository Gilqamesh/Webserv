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
                int cgi_fd = *(int *)events[i].udata;
                assert(cgi_responses.count(cgi_fd) != 0);
                if (sockets.count(cgi_responses[cgi_fd])) /* if we still have connection with the client send the response */
                {
                    std::map<int, std::string> *cgi_out_files = &servers[sockets[cgi_responses[cgi_fd]]].cgi_outfiles;
                    assert(cgi_out_files->count(cgi_fd) != 0);
                    int cgi_out;
                    if (fileIsOpen.count(cgi_fd) == 0)
                    {
                        accumulatedValues[cgi_fd] = 0;
                        cgi_out = open((*cgi_out_files)[cgi_fd].c_str(), O_RDONLY);
                        if (cgi_out == -1)
                            WARN("open failed for reading: " << (*cgi_out_files)[cgi_fd]);
                        fileIsOpen[cgi_fd] = cgi_out;
                        struct stat fileInfo;
                        if (stat((*cgi_out_files)[cgi_fd].c_str(), &fileInfo) == -1)
                            TERMINATE(("stat failed on file: " + (*cgi_out_files)[cgi_fd]).c_str());
                        fileSizes[cgi_fd] = fileInfo.st_size;
                    }
                    else
                    {
                        cgi_out = fileIsOpen[cgi_fd];
                    }
                    char buffer[4096];
                    int readRet = read(cgi_out, buffer, 4096);

                    accumulatedValues[cgi_fd] += send(cgi_responses[cgi_fd], buffer, readRet, 0);
                    WARN("accumulatedValues[cgi_fd]: " << accumulatedValues[cgi_fd]);
                    if (readRet == -1)
                        WARN("read failed");
                    if (accumulatedValues[cgi_fd] == fileSizes[cgi_fd])
                    {
                        WARN("accumulatedValues.erase(cgi_fd): " << accumulatedValues[cgi_fd]);
                        accumulatedValues.erase(cgi_fd);
                        fileIsOpen.erase(cgi_fd);
                        close(cgi_out);
                        cgi_out_files->erase(cgi_fd);
                        /* cut connection with the client */
                        servers[sockets[cgi_responses[cgi_fd]]].cut_connection(cgi_responses[cgi_fd]);
                        /* remove client socket from 'sockets' */
                        sockets.erase(cgi_responses[cgi_fd]);
                        cgi_responses.erase(cgi_fd);
                        if (fileIsOpen.count(cgi_fd))
                            fileIsOpen.erase(cgi_fd);
                        if (accumulatedValues.count(cgi_fd))
                            accumulatedValues.erase(cgi_fd);
                        close(cgi_fd);
                        free(events[i].udata);
                        events[i].udata = NULL;
                        fileSizes.erase(cgi_fd);
                        continue ;
                    }
                    continue ;
                    // else
                    // {
                    //     events.addReadEvent(cgi_fd, (int *)events[i].udata);
                    // }

                    // int accumulatedValue = 0;
                    // while (1)
                    // {
                    //     int readRet = read(cgi_out, buffer, 16384);
                    //     if (readRet == -1)
                    //     {
                    //         WARN("read failed");
                    //         break ;
                    //     }
                    //     if (readRet == 0)
                    //         break ;
                    //     accumulatedValue += send(cgi_responses[cgi_fd], buffer, readRet, 0);
                    //     usleep(10000);
                    //     buffer[readRet] = '\0';
                    // }
                    // WARN("accumulatedValue: " << accumulatedValue);
                    // WARN("response: " << response);
                    // close(cgi_out);
                    // cgi_out_files->erase(cgi_fd);
                    // /* cut connection with the client */
                    // usleep(10000);
                    // servers[sockets[cgi_responses[cgi_fd]]].cut_connection(cgi_responses[cgi_fd]);
                }
                else
                {
                    /* remove client socket from 'sockets' */
                    sockets.erase(cgi_responses[cgi_fd]);
                    cgi_responses.erase(cgi_fd);
                    if (fileIsOpen.count(cgi_fd))
                        fileIsOpen.erase(cgi_fd);
                    if (accumulatedValues.count(cgi_fd))
                        accumulatedValues.erase(cgi_fd);
                    fileSizes.erase(cgi_fd);
                    close(cgi_fd);
                    free(events[i].udata);
                    events[i].udata = NULL;
                    continue ;
                }
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
