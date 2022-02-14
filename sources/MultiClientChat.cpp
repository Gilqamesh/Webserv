#include "MultiClientChat.hpp"
#include <string>

// Handler for client connections
void MultiClientChat::onClientConnected(int clientSocket)
{
	// Send a welcome message to the connected client
	std::string welcomeMsg = "Welcome to the Awesome Chat Server, ";
	int clientSocketInt = getConnection(clientSocket);
	std::stringstream clientSocketStream;
	clientSocketStream << clientSocketInt;
	std::string clientSocketString = clientSocketStream.str();
	welcomeMsg += "client #" + clientSocketString + "!\n";
	sendToClient(clientSocket, welcomeMsg.c_str(), welcomeMsg.size() + 1);
}

// Handler for client disconnections
void MultiClientChat::onClientDisconnected(int clientSocket)
{
	std::cout << "client " << getConnection(clientSocket)  << " disconnected.\n";
}

// handler for when message is received from the client
void MultiClientChat::onMessageReceived(int clientSocket)
{
	char buf[MAX_MSG_SIZE];
	int bytes_read = recv(clientSocket, buf, sizeof(buf) - 1, 0);
	buf[bytes_read] = 0;
	printf("client #%d: %s", getConnection(clientSocket), buf);
	fflush(stdout);
	broadcastToClients(clientSocket);
}
