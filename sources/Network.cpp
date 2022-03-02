#include "Network.hpp"

void Network::initNetwork(char *file_name)
{
	// the kqueue holds all the events we are interested in
    // to start, we simply create an empty kqueue
    conf_file               config_file(file_name);
    std::vector<t_server>   configs = config_file.get_configs();
    server                  serverNetwork[configs.size()];

    if ((kq = kqueue()) == -1)
        TERMINATE("failed to create empty kqueue");

	start_timestamp_network = get_current_timestamp();

    for (size_t i = 0; i < configs.size(); i++)
    {
	    serverNetwork[i].construct(configs[i].port, 10, start_timestamp_network, &cgi_responses);
        for (size_t j = 0; j < configs[i].locations.size(); j++)
	        serverNetwork[i].cache_file(configs[i].locations[j].root + "/" + configs[i].locations[j].index, configs[i].locations[j].route);
	    servers.insert(std::pair<int, server>(serverNetwork[i].getServerSocketFd(), serverNetwork[i]));
    }
}

/*
* main loop of the servers
* waits for new connections or handles already established connections
*/

void Network::runNetwork()
{
    // EV_SET() -> initialize a kevent structure, here our listening server
    /* No EV_CLEAR, otherwise when backlog is full, the connection request has to
    * be repeated.. instead let them hang in the queue until they are served
    */
	for (std::map<int,server>::iterator it = servers.begin(); it != servers.end(); ++it)
	{
		EV_SET(&event, (*it).first, EVFILT_READ, EV_ADD /*| EV_CLEAR */, 0, 0, NULL);

		// ...and register it to the kqueue;
		if (kevent(kq, &event, 1, NULL, 0, 0) == -1)
			TERMINATE("Registration failed");
	}

	bool accepted = false;
    while (true)
    {
        // call kevent(..) to receive incoming events and process them
        // waiting/reading events (up to N (N = MAX_EVENTS) at a time)
        // returns number of events
        int nev = kevent(kq, NULL, 0, evList, MAX_EVENTS, NULL);

        for (int i = 0; i < nev; ++i)
        {
            int fd = static_cast<int>(evList[i].ident);
            if (evList[i].udata != NULL) /* CGI socket */
            {
                assert(cgi_responses.count(*(int *)evList[i].udata) != 0);
                if (sockets.count(cgi_responses[*(int *)evList[i].udata])) /* if we still have connection with the client send the response */
                {
                    std::string response;
                    std::string tmp;
                    while ((tmp = get_next_line(*(int *)evList[i].udata)).length())
                        response += tmp;
                    send(cgi_responses[*(int *)evList[i].udata], response.data(), response.length(), 0);
                }
                sockets.erase(cgi_responses[*(int *)evList[i].udata]);
                servers[cgi_responses[*(int *)evList[i].udata]].cut_connection(kq, cgi_responses[*(int *)evList[i].udata], &event);
                cgi_responses.erase(*(int *)evList[i].udata);
                close(*(int *)evList[i].udata);
                continue ;
            }
			for (std::map<int,server>::iterator it = servers.begin(); it != servers.end(); ++it)
			{
				if (fd == (*it).first)
                {
                    accepted = true;
                    break;
                }
			}
			if (accepted == true)                 /* receive new connection */
			{
				int new_socket = servers[fd].accept_connection(kq, fd, &event);
				sockets.insert(std::pair<int,int>(new_socket, fd));
			}
			else if (evList[i].filter == EVFILT_READ)   /* socket is ready to be read */
            {
                if (evList[i].flags & EV_EOF) /* client side shutdown */
                {
                    servers[sockets[fd]].cut_connection(kq, fd, &event);
                    sockets.erase(fd);
                    continue ;
                }
                /* update socket's timeout event */
                EV_SET(&event, fd, EVFILT_TIMER, EV_ADD, 0, TIMEOUT_TO_CUT_CONNECTION * 1000, NULL);
                kevent(kq, &event, 1, NULL, 0, NULL);
                /* handle connection */
            	servers[sockets[fd]].handle_connection(kq, fd, &event);
            }
            else if (evList[i].filter == EVFILT_TIMER)  /* socket is expired */
            {
				servers[sockets[fd]].send_timeout(fd); /* 408 Request Timeout */
                servers[sockets[fd]].cut_connection(kq, fd, &event);
                sockets.erase(fd);
            }
			accepted = false;
        }
    }
}

Network::Network() {}

Network::~Network()
{
    // ToDo: cut connection of servers
}
