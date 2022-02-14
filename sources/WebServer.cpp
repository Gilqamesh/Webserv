#include "WebServer.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <iterator>

// handler for when message is received from the client
void WebServer::onMessageReceived(int clientSocket)
{
	// Send connection message to terminal
	char buf[4096];
	memset(buf, 0, sizeof(buf));
	// int bytes_read = recv(clientSocket, buf, 4096, 0);
	recv(clientSocket, buf, 4096, 0);
	printf("client #%d: %s\n", getConnection(clientSocket), buf);
	fflush(stdout);
	
	
	// GET /index.html HTTP/1.1

	// Parse out the client's request string e.g. GET /index.html HTTP/1.1
	// parse by spaces
	std::istringstream iss(buf);
	std::vector<std::string> parsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

	// Some defaults for output to the client (404 file not found 'page')
	std::string content = "<h1>404 Not Found</h1>";
	std::string htmlFile = "/index.html";
	int errorCode = 404;

	// If the GET request is valid, try and get the name
	if (parsed.size() >= 3 && parsed[0] == "GET")
	{
		htmlFile = parsed[1];

		// If the file is just a slash, use index.html. This should really
		// be if it _ends_ in a slash. I'll leave that for you :)
		if (htmlFile == "/")
		{
			htmlFile = "/index.html";
		}
	}

	// Open the document in the local file system
	std::ifstream f("views" + htmlFile);

	// Check if it opened and if it did, grab the entire contents
	if (f.good())
	{
		std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		content = str;
		errorCode = 200;
	}

	f.close();

	// Write the document back to the client
	std::ostringstream oss;
	oss << "HTTP/1.1 " << errorCode << " OK\r\n";
	oss << "Cache-Control: no-cache, private\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << content.size() << "\r\n";
	oss << "\r\n";
	oss << content;

	std::string output = oss.str();
	int size = output.size() + 1;

	sendToClient(clientSocket, output.c_str(), size);
}

// Handler for client connections
void WebServer::onClientConnected(int clientSocket)
{
	(void) clientSocket;
}

// Handler for client disconnections
void WebServer::onClientDisconnected(int clientSocket)
{
	std::cout << "client " << getConnection(clientSocket)  << " disconnected.\n";
}
