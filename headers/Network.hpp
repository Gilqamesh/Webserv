#ifndef NETWORK_HPP
# define NETWORK_HPP

# include "server.hpp"
# include "utils.hpp"
# include <set>
# include <map>
# include "conf_file.hpp"

# include <sys/types.h> // kqueue, kevent
# include <sys/event.h>
# include <sys/time.h>

# define MAX_EVENTS 32 // for kqueue 

class server;

class Network
{
	public:
		Network();
		~Network();
		void initNetwork(char *file_name);
		void runNetwork();

		int				kq; /* holds all the events we are interested in */
		struct kevent 	event;
		struct kevent 	evList[MAX_EVENTS];
		unsigned long	start_timestamp_network;

	private:
		std::map<int,server>	servers; // server_socket_fd - server
		std::map<int,int>		sockets; // socket - server_socket_fd
		
		Network(Network const &Network);
		Network &operator=(Network const &Network);
};


#endif