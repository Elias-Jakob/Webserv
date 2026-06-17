1.	*SERVER starts*
	* READS the CONFIGURATION FILE.
	* SET VALUES from configuration file.

2. *CREATE SOCKET*
	* getaddrinfo()
	* socket()
	* setsockopt()
	* bind
	* listen()
	* fcntl()

getaddrinfo -> int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

3.	*CREATE NEEDED DATA*
	* std::vector<struct pollfd> fds.
	* struct pollfd pfd;
		- pfd.fd = socket_fd;
		- pfd.events = POLLIN;
		- pfd.revents = 0;
		- fds.push_back(pfd);
	* std::map<int, ClientConnection> clients

3. *LOOP TO HANDLE CONNECTIONS FROM CLIENTS*
	* poll(&fds[0], fds.size(), -1); // waits for activity on any socket.
	* LOOP through fds.
		- if (fds[i].revents == 0) => continue;
		- if (fds[i].fd == socket_fd) => handleNewConnection();
		- else if (fds[i].revents & POLLIN) => handleClientRead();
		- else if (fds[i].revents & POLLOUT) => handleClientWrite();

1. SERVER start
2. set settings for server by CONFIG FILE
3. SOCKET handling
4. CONNECTION handling
5. REQUEST received
6. REQUEST parsing
7. RESPONSE class creation
8. decide METHOD && construct it
	*	GET
	*	DELETE
	*	POST
		create special parser for Body parsing
9. EXECUTE Method
10. BUILD Response text
11. SEND Response to client


* FLOW
get message, and parses it into a simple structure.
MethodExecuter -> parses Request into simple data structure
