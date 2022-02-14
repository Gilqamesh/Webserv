#include "TcpListener.hpp"

// To store an array of client_data (which contains the socket’s fd), and initially all of them have fd = 0, which means “unused”.
struct client_data
{
	int fd;
} clients[NUM_CLIENTS];


int TcpListener::init()
{
	// 1. Create a socket
	// 2. Bind it to an address and a port
	// 3. Listen on the socket for incoming connections
	
	struct addrinfo *addr;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(m_ipAddress, m_port, &hints, &addr);
	int local_s = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	bind(local_s, addr->ai_addr, addr->ai_addrlen);
	listen(local_s, 5);
	return local_s;
}

int TcpListener::run(int kq, int local_s)
{
	struct kevent evSet;
	struct kevent evList[MAX_EVENTS];
	struct sockaddr_storage addr;
	socklen_t socklen = sizeof(addr);

	while (1) 
	{
		// call kevent(..) to receive incoming events and process them
		// waiting/reading events (up to N (N = MAX_EVENTS) at a time)
		int num_events = kevent(kq, NULL, 0, evList, MAX_EVENTS, NULL);

		for (int i = 0; i < num_events; i++)
		{
			// receive new connection
			if ((int)evList[i].ident == local_s)
			{
				// Each time we receive a new connection from a client, we accept the connection. The accept(..) system call basically 
				// does the tcp 3-way handshake and creates a socket for further communication with that client and returns the file 
				// descriptor of that socket. 
				int fd = accept(evList[i].ident, (struct sockaddr *) &addr, &socklen);

				// We need to store these file descriptors for each client so we can communicate with them (in struct "client_data")
				if (addConnection(fd) == 0)
				{
					// EV_SET() -> initialize a kevent structure.
					EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

					// Register for the incoming messages from that client (on the same kqueue) 
					kevent(kq, &evSet, 1, NULL, 0, NULL);

					// Send a welcome message on new socket!
					onClientConnected(fd);
				} 
				else
				{
					printf("connection refused.\n");
					close(fd);
				}
			} // client disconnected
			else if (evList[i].flags & EV_EOF) 
			{
				// When a client disconnects, we receive an event where the EOF flag is set on the socket.
				// We simply free up that connection in the pool and remove the event from kqueue (via EV_DELETE).
				int fd = evList[i].ident;
				onClientDisconnected(fd);
				EV_SET(&evSet, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
				kevent(kq, &evSet, 1, NULL, 0, NULL);
				deleteConnection(fd);
			} // read message from client
			else if (evList[i].filter == EVFILT_READ) 
			{
				// Handle incoming data from clients and receive their message.
				onMessageReceived(evList[i].ident);
			}
		}
	}
	return 0;
}

void TcpListener::sendToClient(int clientSocket, const char* msg, int length)
{
	send(clientSocket, msg, length, 0);
}

void TcpListener::broadcastToClients(int clientSocket)
{
	std::string response = "Thanks for the request, ";
	int clientSocketInt = getConnection(clientSocket);
	std::stringstream clientSocketStream;
	clientSocketStream << clientSocketInt;
	std::string clientSocketString = clientSocketStream.str();
	response += "client #" + clientSocketString + "!\n";
	sendToClient(clientSocket, response.c_str(), response.size() + 1);
}

int TcpListener::getConnection(int fd) 
{
	for (int i = 0; i < NUM_CLIENTS; i++)
		if (clients[i].fd == fd)
			return i;
	return -1;
}

int TcpListener::addConnection(int fd)
{
	if (fd < 1)
		return -1;
	int i = getConnection(0);
	if (i == -1)
		return -1;
	clients[i].fd = fd;
	return 0;
}

int TcpListener::deleteConnection(int fd) 
{
	if (fd < 1)
		return -1;
	int i = getConnection(fd);
	if (i == -1) 
		return -1;
	clients[i].fd = 0;
	return close(fd);
}