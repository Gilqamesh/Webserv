#ifndef NETWORK_HPP
# define NETWORK_HPP

# include "EventHandler.hpp"
# include "server.hpp"
# include "utils.hpp"
# include <set>
# include <map>
# include "conf_file.hpp"

# define BACKLOG    200

class server;

class Network
{
	public:
		Network();
		~Network();
		void initNetwork(char *file_name);
		void runNetwork();

		EventHandler	events;
		unsigned long	start_timestamp_network;

    // 100.000.000
	private:
		std::map<int, server>		servers; // server_socket_fd - server
		std::map<int, int>			sockets; // socket - server_socket_fd
		std::map<int, int>			cgi_responses; /* cgi socket -> client socket */
        std::map<int, int>          fileIsOpen; /* socket - file fd */
        std::map<int, size_t>       fileSizes;
        // debug
        std::map<int, size_t>          accumulatedValues;

		Network(Network const &Network);
		Network &operator=(Network const &Network);
};


#endif