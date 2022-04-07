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
		std::map<int, server>		serverSocketToServer; // server_socket_fd - server
		std::map<int, int>			clientToServerSocket; // client socket - server_socket_fd
		std::map<int, int>			cgiToClientSockets; /* cgi socket -> client socket */
        std::map<int, int>          fileIsOpen; /* client socket - file fd */
        std::map<int, std::string>  fileNames; /* file fd - file name */
        std::map<int, size_t>       fileSizes; /* client socket - file size */
        // debug
        std::map<int, size_t>       accumulatedValues; /* client socket - sent data so far */

		Network(Network const &Network);
		Network &operator=(Network const &Network);
};


#endif