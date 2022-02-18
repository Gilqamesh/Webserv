#include "Network.hpp"

void Network::initNetwork()
{
	// the kqueue holds all the events we are interested in
    // to start, we simply create an empty kqueue
    if ((this->kq = kqueue()) == -1)
        TERMINATE("failed to create empty kqueue");



	// init servers.......
}


void Network::runNetwork()
{
	while (true)
	{
		// call kevent(..) to receive incoming events and process them
		// waiting/reading events (up to N (N = MAX_EVENTS) at a time)
		// returns number of events
		int nev = kevent(kq, NULL, 0, evList, MAX_EVENTS, NULL);

		for (int i = 0; i < nev; ++i)
		{
			int fd = static_cast<int>(evList[i].ident);
			if (fd == server_socket_fd)                 // receive new connection
				accept_connection(fd);
			else if (evList[i].filter == EVFILT_READ)
			{
				if (evList[i].flags & EV_EOF)          // client disconnected
					cut_connection(fd);
				else
					handle_connection(fd);
			}
			else if (evList[i].filter == EVFILT_WRITE)
			{
				// TODO -> handle EVFILT_WRITE
			}
			else if (evList[i].filter == EVFILT_TIMER)  // check for timeout
				cut_connection(fd);
			else if (evList[i].flags & EV_EOF)          // client disconnected
				cut_connection(fd);
		}
	}
}
























Network::Network() 
{
	// read/handle config file
}

Network::~Network() {}

Network::Network(Network const &rhs) {*this = rhs;}

Network &Network::operator=(Network const &rhs)
{
	if (this != &rhs)
	{
		this->_servers = rhs._servers;
		this->kq = rhs.kq;
	}
	return *this;
}