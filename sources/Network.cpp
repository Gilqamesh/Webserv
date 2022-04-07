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
	    serverNetwork[i].construct(configs[i].port, BACKLOG, start_timestamp_network, &cgiToClientSockets, &events, configs[i]);
	    serverSocketToServer.insert(std::pair<int, server>(serverNetwork[i].getServerSocketFd(), serverNetwork[i]));
    }
}

/*
* main loop of the server
* waits for new connections or handles already established connections
*/
void Network::runNetwork()
{
	for (std::map<int, server>::iterator it = serverSocketToServer.begin(); it != serverSocketToServer.end(); ++it)
        events.addReadEvent(it->first);

    while (true)
    {
        int nev = events.getNumberOfEvents();

        for (int i = 0; i < nev; ++i)
        {
            int fd = events[i].ident;
            if (events[i].filter == EVFILT_WRITE) /* for CGI if the socket can be read */
            {
                int amountOfSpace = events[i].data > 16384 ? 16384 : events[i].data;
                assert(fileIsOpen.count(fd));
                if (clientToServerSocket.count(fd)) /* if we still have connection with the client send the response */
                {
                    char buffer[16384];
                    int readRet = read(fileIsOpen[fd], buffer, amountOfSpace);
                    if (readRet == -1)
                        TERMINATE("send failed");
                    int sendRet = send(fd, buffer, readRet, 0);
                    if (sendRet == -1)
                        TERMINATE("send failed");
                    accumulatedValues[fd] += sendRet;
                    WARN("accumulatedValues[fd]: " << accumulatedValues[fd]);
                    if (accumulatedValues[fd] == fileSizes[fd]) /* check if we are done */
                    {
                        accumulatedValues.erase(fd);
                        cgiToClientSockets.erase(fd);
                        fileSizes.erase(fd);
                        events.removeWriteEvent(fd);
                        serverSocketToServer[clientToServerSocket[fd]].cut_connection(fd);
                        clientToServerSocket.erase(fd);
                        std::remove(fileNames[fileIsOpen[fd]].c_str());
                        fileNames.erase(fileIsOpen[fd]);
                        close(fileIsOpen[fd]);
                        fileIsOpen.erase(fd);
                        continue ;
                    }
                    continue ;
                }
                else /* remove all associated objects with client */
                {
                    accumulatedValues.erase(fd);
                    cgiToClientSockets.erase(fd);
                    fileSizes.erase(fd);
                    events.removeWriteEvent(fd);
                    serverSocketToServer[clientToServerSocket[fd]].cut_connection(fd);
                    clientToServerSocket.erase(fd);
                    if (fileIsOpen.count(fd))
                    {
                        std::remove(fileNames[fileIsOpen[fd]].c_str());
                        fileNames.erase(fileIsOpen[fd]);
                        close(fileIsOpen[fd]);
                        fileIsOpen.erase(fd);
                    }
                    WARN("Lost connection to client");
                }
                continue ;
            }
            else if (events[i].udata != NULL) /* CGI fd, should only happen 1 time, the rest is handled with Write Event */
            {
                assert(cgiToClientSockets.count(fd) != 0);
                int clientSocketFd = cgiToClientSockets[fd];
                std::map<int, std::string> *cgi_out_files = &serverSocketToServer[clientToServerSocket[clientSocketFd]].cgi_outfiles;
                assert(cgi_out_files->count(fd) != 0);
                if (clientToServerSocket.count(clientSocketFd)) /* if we still have connection with the client send the response */
                {
                    if (fileIsOpen.count(clientSocketFd) == 0)
                    {
                        accumulatedValues[clientSocketFd] = 0;
                        fileIsOpen[clientSocketFd] = open((*cgi_out_files)[fd].c_str(), O_RDONLY);
                        fileNames[fileIsOpen[clientSocketFd]] = (*cgi_out_files)[fd];
                        if (fileIsOpen[clientSocketFd] == -1)
                            WARN("open failed for reading: " << (*cgi_out_files)[fd]);
                        struct stat fileInfo;
                        if (stat((*cgi_out_files)[fd].c_str(), &fileInfo) == -1)
                            TERMINATE(("stat failed on file: " + (*cgi_out_files)[fd]).c_str());
                        fileSizes[clientSocketFd] = fileInfo.st_size;
                    }
                    char buffer[16384];
                    int readRet = read(fileIsOpen[clientSocketFd], buffer, 16384);

                    WARN("accumulatedValues[fd]: " << accumulatedValues[clientSocketFd]);
                    if (readRet == -1)
                        WARN("read failed");
                    int sendRet = send(clientSocketFd, buffer, readRet, 0);
                    if (sendRet == -1)
                        WARN("send failed");
                    accumulatedValues[clientSocketFd] += sendRet;
                    if (accumulatedValues[clientSocketFd] == fileSizes[clientSocketFd])
                    {
                        WARN("accumulatedValues.erase(fd): " << accumulatedValues[clientSocketFd]);
                        accumulatedValues.erase(clientSocketFd);
                        cgi_out_files->erase(fd);
                        /* cut connection with the client */
                        /* remove client socket from 'clientToServerSocket' */
                        cgiToClientSockets.erase(fd);
                        fileSizes.erase(clientSocketFd);
                        events.removeReadEvent(fd);
                        serverSocketToServer[clientToServerSocket[clientSocketFd]].cut_connection(clientSocketFd);
                        clientToServerSocket.erase(clientSocketFd);
                        std::remove(fileNames[fileIsOpen[clientSocketFd]].c_str());
                        fileNames.erase(fileIsOpen[clientSocketFd]);
                        close(fileIsOpen[clientSocketFd]);
                        fileIsOpen.erase(clientSocketFd);
                        free(events[i].udata);
                        events[i].udata = NULL;
                        continue ;
                    }
                    free(events[i].udata);
                    events[i].udata = NULL;
                    events.removeReadEvent(fd);
                    events.addWriteEvent(clientSocketFd);
                    continue ;
                }
                else /* if we no longer have connection with the client but there is stuff to send from cgi */
                {
                    cgi_out_files->erase(fd);
                    cgiToClientSockets.erase(fd);
                    if (accumulatedValues.count(fd))
                        accumulatedValues.erase(fd);
                    fileSizes.erase(fd);
                    close(fd);
                    events.removeReadEvent(fd);
                    serverSocketToServer[clientToServerSocket[clientSocketFd]].cut_connection(clientSocketFd);
                    clientToServerSocket.erase(clientSocketFd);
                    if (fileIsOpen.count(clientSocketFd))
                    {
                        std::remove(fileNames[fileIsOpen[clientSocketFd]].c_str());
                        fileNames.erase(fileIsOpen[clientSocketFd]);
                        close(fileIsOpen[clientSocketFd]);
                        fileIsOpen.erase(clientSocketFd);
                    }
                    free(events[i].udata);
                    events[i].udata = NULL;
                    WARN("Lost connection to client");
                    continue ;
                }
            }
            if (serverSocketToServer.count(fd)) /* if 'fd' is a server socket -> accept connection */
            {
                int new_socket;
                /* if could not accept connection continue to the next event */
                if ((new_socket = serverSocketToServer[fd].accept_connection()) == -1)
                    continue ;
                /* otherwise map the socket to the server socket */
                clientToServerSocket[new_socket] = fd;
            }
			else if (events[i].filter == EVFILT_READ)   /* socket is ready to be read */
            {
                if (events[i].flags & EV_EOF) /* client side shutdown */
                {
                    serverSocketToServer[clientToServerSocket[fd]].cut_connection(fd);
                    clientToServerSocket.erase(fd);
                    continue ;
                }
                /* update socket's timeout event */
                events.addTimeEvent(fd, TIMEOUT_TO_CUT_CONNECTION * 1000);
                /* handle connection */
            	serverSocketToServer[clientToServerSocket[fd]].handle_connection(fd);
            }
            else if (events[i].filter == EVFILT_TIMER)  /* socket is expired */
            {
				serverSocketToServer[clientToServerSocket[fd]].send_timeout(fd); /* 408 Request Timeout */
                serverSocketToServer[clientToServerSocket[fd]].cut_connection(fd);
                clientToServerSocket.erase(fd);
            }
        }
    }
}

Network::Network() {}

Network::~Network()
{
    // ToDo: cut connection of servers
}
