# include "Server.hpp"

/*
 * @brief Handle incoming connection requests.
 *
 * Accept the connection request with accept(), creating a
 * new fd:ClientConnection pair in this->clients and adds the
 * client socket fd to the epoll interest list.
 *
 * @param listenFd the listening socket's fd
 * @return
 * @note
 **/
void	Server::handleNewClient(int listenFd, MethodExecuter &methodExecuter, ResponseBuilder &responseBuilder)
{
	int	fd;
	struct epoll_event	epEvent;

	fd = accept(listenFd, NULL, NULL);
	if (fd == -1)
		throw std::runtime_error(std::strerror(errno));
	std::cout << "New client connected... socket file descriptor = " << fd << " ";
	printSocketInfo(fd);
	fcntl(fd, F_SETFL, O_NONBLOCK); // make it nonblocking

	// Initialize client connection directly in map (avoid copy issues)
	//this->clients[fd] = ClientConnection();
	this->clients[fd].fd = fd;
	this->clients[fd].state = READING_REQUEST;
	this->clients[fd].request = new HttpRequest();
	this->clients[fd].executor = &methodExecuter;
	this->clients[fd].responseBuilder = &responseBuilder;
	this->clients[fd].bytes_sent = 0;

	// TODO: clean up
	// this->clients[fd] = ClientConnection(fd);
	epEvent.events = EPOLLIN;
	epEvent.data.fd = fd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &epEvent) == -1)
		throw std::runtime_error(std::strerror(errno));
}

void	Server::handleClientRead(int clientFd)
{
	struct epoll_event	epEvent;

	std::cout << "ClientRead() for fd: " << clientFd << std::endl;
	char buffer[4096];
	ssize_t bytes = recv(clientFd, buffer, sizeof(buffer), 0);

	if (bytes < 0)
	{
		// Non-blocking socket: EAGAIN/EWOULDBLOCK means no data available yet
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			std::cout << "No data available yet (non-blocking)" << std::endl;
			return;  // Not an error, just try again later
		}
		// Real error
		close(clientFd);
		clients.erase(clientFd);
		// INFO: i dont think that it is necessary to close & erase the client here
		throw std::runtime_error(std::strerror(errno));
	}
	else if (bytes == 0)
	{
		// Client closed connection
		std::cout << "Client closed connection" << std::endl;
		close(clientFd);
		clients.erase(clientFd);
		return;
	}

	std::cout << "Received " << bytes << " bytes from " << clientFd << std::endl;

	if (clients[clientFd].request == NULL)// Check if request pointer is valid
	{
		std::cerr << "ERROR: request pointer is NULL!" << std::endl;
		close(clientFd);
		clients.erase(clientFd);
		return;
	}

	// Parse request - use string constructor with length to avoid buffer overflow
	std::string request_data(buffer, bytes);
	clients[clientFd].request->parseRequest(request_data);
	if (clients[clientFd].request->parsingComplete())
	{
		clients[clientFd].state = PROCESSING;
		clients[clientFd].processRequest();
		clients[clientFd].state = SENDING_RESPONSE;

		epEvent.events = EPOLLIN | EPOLLOUT;
		epEvent.data.fd = clientFd;
		if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
			throw std::runtime_error(std::strerror(errno));
	}
}

void	Server::handleClientWrite(int clientFd)
{
	struct epoll_event	epEvent;


	ssize_t		sent;

	std::cout << "\033[35m==========\nRESPONSE sending...\n" << std::endl;
	sent = send(clientFd, clients[clientFd].response_buffer.c_str(), clients[clientFd].response_buffer.size(), 0);
	
	if (sent < 0)
	{
		// Non-blocking socket: EAGAIN/EWOULDBLOCK means can't send right now
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			std::cout << "Socket not ready for writing, try again later" << std::endl;
			return;  // Keep connection open, try again on next POLLOUT
		}
		// Real error
		std::cout << "send() error: " << strerror(errno) << std::endl;
		close(clientFd);
		clients.erase(clientFd);
		throw std::runtime_error(std::strerror(errno));
	}
	
	std::cout << "bytes sent: " << sent << "\n==========\033[m" << std::endl;
	std::cout << "response_buffer: " << clients[clientFd].response_buffer << std::endl;
	// Clean up: close connection and remove from tracking
	if (clients[clientFd].keep_alive == false)
	{
		std::cout << "Client connection is not set to keep-alive, closing socket..." << std::cout;
		close(clientFd);
		clients.erase(clientFd);  // This will call destructor and free request/response
		return;
	}
	epEvent.events = EPOLLIN;
	epEvent.data.fd = clientFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
		throw std::runtime_error(std::strerror(errno));
	clients[clientFd].cleanUpClient();
}

bool	Server::isListenSock(int fd)
{
	for (std::vector<int>::iterator	it = this->listenSockets.begin();
			it != this->listenSockets.end(); ++it) {
		if (*it == fd)
			return (true);
	}
	return (false);
}

void	Server::eventLoop()
{
	int	n = 1, nReady;
	struct epoll_event	ev[n];
	MethodExecuter	methodExecuter;
	ResponseBuilder	responseBuilder;

	while (sigFlag != SIGINT)//true)
	{
		if ((nReady = epoll_wait(this->epollFd, ev, n, -1)) == -1)
			throw std::runtime_error(std::strerror(errno));
		for (int	i = 0; i < nReady; ++i) {
			if (this->isListenSock(ev[i].data.fd))
				this->handleNewClient(ev[i].data.fd, methodExecuter, responseBuilder);
			else if (ev[i].events & EPOLLIN) // the client is available for read
				this->handleClientRead(ev[i].data.fd);
			else if (ev[i].events & EPOLLOUT) // the client is available for write
				this->handleClientWrite(ev[i].data.fd);
		}
	}
}
