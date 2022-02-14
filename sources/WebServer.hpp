#pragma once

#include "TcpListener.hpp"

class WebServer : public TcpListener
{
public:
	WebServer(const char *ipAdress, const char *port) :
		TcpListener(ipAdress, port) {}

protected:
	// Handler for client connections
	virtual void onClientConnected(int clientSocket);

	// Handler for client disconnections
	virtual void onClientDisconnected(int clientSocket);

	// handler for when message is received from the client
	virtual void onMessageReceived(int clientSocket);
};