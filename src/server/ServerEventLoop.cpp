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
	std::cout << "New client connected... socket file descriptor = " << fd << std::endl;
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) // make it nonblocking
		throw std::runtime_error(std::strerror(errno));

	// Initialize client connection directly in map (avoid copy issues)
	//this->clients[fd] = ClientConnection();
	this->clients[fd].fd = fd;
	this->clients[fd].state = READING_REQUEST;
	this->clients[fd].request = new HttpRequest();
	this->clients[fd].executor = &methodExecuter;
	this->clients[fd].responseBuilder = &responseBuilder;
	this->clients[fd].bytesSent = 0;

	// TODO: clean up
	// this->clients[fd] = ClientConnection(fd);
	epEvent.events = EPOLLIN;
	epEvent.data.fd = fd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &epEvent) == -1)
		throw std::runtime_error(std::strerror(errno));
}

void	Server::handleClientRead(int clientFd)
{
	ssize_t	bytesRecv;
	char buffer[4096];
	struct epoll_event	epEvent;

	std::cout << "ClientRead() for fd: " << clientFd << std::endl;
	bytesRecv = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRecv == -1)
		throw std::runtime_error(std::strerror(errno));
	else if (bytesRecv == 0)
	{
		// Client closed connection
		std::cout << "Client closed connection" << std::endl;
		close(clientFd);
		this->clients.erase(clientFd);
		return;
	}
	std::cout << "Received " << bytesRecv << " bytes from " << clientFd << std::endl;
	if (clients[clientFd].request == NULL)// Check if request pointer is valid
	{
		std::cerr << "ERROR: request pointer is NULL!" << std::endl;
		close(clientFd);
		clients.erase(clientFd);
		return;
	}
	// Parse request - use string constructor with length to avoid buffer overflow
	std::string request_data(buffer, bytesRecv);
	clients[clientFd].request->parseRequest(request_data);
	if (clients[clientFd].request->parsingComplete())
	{
		clients[clientFd].state = PROCESSING;
		clients[clientFd].processRequest();
		// if (clients[clientFd].state != CGI_PROCESSING)
		clients[clientFd].state = SENDING_RESPONSE;
		std::cout << "hello " << clients[clientFd].cgi_path << std::endl;
		this->launchCGI(clients[clientFd]);

		epEvent.events = EPOLLIN | EPOLLOUT;
		epEvent.data.fd = clientFd;
		if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
			throw std::runtime_error(std::strerror(errno));
	}
}

void	Server::handleClientWrite(int clientFd)
{
	ClientConnection	&client = this->clients.at(clientFd);
	ssize_t		sentBytes;
	struct epoll_event	epEvent;

	std::cout << "\033[35m==========\nRESPONSE sending...\n" << std::endl;
	sentBytes = send(clientFd, client.response_buffer.c_str(), client.response_buffer.size(), 0);
	if (sentBytes == -1)
		throw std::runtime_error(std::strerror(errno));
	client.bytesSent += sentBytes; // TODO:
	if (client.bytesSent < client.response_buffer.size()) {
		std::cout << "bytes sent: " << sentBytes << " response_buffer: " << client.response_buffer.size() << " was not fully sent" << std::endl;
		return;
	}
	std::cout << "bytes sent: " << sentBytes << "\n==========\033[m" << std::endl;
	// std::cout << "response_buffer: " << client.response_buffer << std::endl;
	// Clean up: close connection and remove from tracking
	if (client.keep_alive == false)
	{
		std::cout << "Client connection is not set to keep-alive, closing socket..." << std::endl;
		close(clientFd);
		this->clients.erase(clientFd);  // This will call destructor and free request/response
		return;
	}
	epEvent.events = EPOLLIN;
	epEvent.data.fd = clientFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
		throw std::runtime_error(std::strerror(errno));
	client.cleanUpClient();
}

void	Server::removeInactiveClients()
{
	for (std::map<int, ClientConnection>::iterator	it = this->clients.begin();
			it != this->clients.end(); ) {
		it->second.inactiveTime++;
		if (it->second.inactiveTime >= KEEP_ALIVE_TIMEOUT) {
			std::cout << "Removed inactive client after timeout... fd = " << it->first << std::endl;
			close(it->first);
			this->clients.erase(it++);
		}
		else ++it;
	}
}

void	Server::eventLoop()
{
	int	nFds;
	struct epoll_event	events[EPOLL_MAX_EVENTS];

	while (sigFlag != SIGINT)
	{
		if ((nFds = epoll_wait(this->epollFd, events, EPOLL_MAX_EVENTS, 1000)) == -1)
			throw std::runtime_error(std::strerror(errno));
		for (int	i = 0; i < nFds; ++i) {
			if (std::find(this->listenSockets.begin(), this->listenSockets.end(),
					events[i].data.fd) != this->listenSockets.end()) {
				this->handleNewClient(events[i].data.fd, methodExecuter, responseBuilder);
				continue;
			}
			
			if (events[i].events & (EPOLLHUP | EPOLLERR)) { // client was disconnected
				close(events[i].data.fd);
				this->clients.erase(events[i].data.fd);
				continue;
			}
			this->clients.at(events[i].data.fd).inactiveTime = 0;
			if (events[i].events & EPOLLIN) // the client is available for read
				this->handleClientRead(events[i].data.fd);
			if (events[i].events & EPOLLOUT) // the client is available for write
				this->handleClientWrite(events[i].data.fd);
		}
		this->removeInactiveClients();
	}
}
