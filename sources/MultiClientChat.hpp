#pragma once

#include "TcpListener2.hpp"

class MultiClientChat : public TcpListener2
{
public:
	MultiClientChat(const char *ipAdress, const char *port) :
		TcpListener2(ipAdress, port) {}

protected:
	// Handler for client connections
	virtual void onClientConnected(int clientSocket);

	// Handler for client disconnections
	virtual void onClientDisconnected(int clientSocket);

	// handler for when message is received from the client
	virtual void onMessageReceived(int clientSocket);
};