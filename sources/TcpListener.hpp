#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_CLIENTS 10
#define MAX_EVENTS 32
#define MAX_MSG_SIZE 256

class TcpListener
{
public:
	// Initialize internal values
	TcpListener(const char *ipAddress, const char *port) :
		m_ipAddress(ipAddress), m_port(port) {}
	
	// Initialize the listener
	int init();
	
	// Run the listener
	int run(int kq, int local_s);

	int m_socket;		// Internal FD for the listening socket

protected:

	// Handler for client connections
	virtual void onClientConnected(int clientSocket) { (void) clientSocket; };

	// Handler for client disconnections
	virtual void onClientDisconnected(int clientSocket) { (void) clientSocket; };

	// handler for when message is received from the client
	virtual void onMessageReceived(int clientSocket) { (void) clientSocket; };

	// Lookup: Given a fd, find the corresponding client_data by iterating over the array
	int getConnection(int fd);

	// Add: For new connections, find the first free item (fd = 0) in the array to store the client’s fd
	int addConnection(int fd);

	// Delete: When the connection is lost, free that item in the array by setting it’s fd to 0
	int deleteConnection(int fd);

	// Send a message to a client
	void sendToClient(int clientSocket, const char *msg, int length);

	// Broadcast a message from a client
	void broadcastToClients(int sendingCLient);

private:
	const char*	m_ipAddress;	// IP Address server will run on
	const char*	m_port;			// Port # for the web service
};