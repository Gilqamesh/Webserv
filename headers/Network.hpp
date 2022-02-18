#ifndef NETWORK_HPP
# define NETWORK_HPP

# include "server.hpp"
# include <set>

# include <sys/types.h> // kqueue, kevent
# include <sys/event.h>
# include <sys/time.h>

# define MAX_EVENTS 32 // for kqueue -> better put in config file?

class Network
{
	public:
		Network();
		~Network();
		Network(Network const &Network);
		Network &operator=(Network const &Network);
	
	protected:
		server	*_servers;
		int					kq; /* holds all the events we are interested in */
		struct kevent                                   evSet;
		struct kevent                                   evList[MAX_EVENTS];
		
		void				initNetwork();
		void				runNetwork();

};


#endif