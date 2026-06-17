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
void	Server::handleNewClient(int listenFd)
{
	int	fd;
	struct epoll_event	epEvent;

	fd = accept(listenFd, NULL, NULL);
	if (fd == -1)
		throw std::runtime_error(std::strerror(errno));

	// Initialize client connection directly in map (avoid copy issues)
	this->clients[fd] = ClientConnection();
	this->clients[fd].fd = fd;
	this->clients[fd].state = READING_REQUEST;
	this->clients[fd].request = new HttpRequest();
	this->clients[fd].response = NULL;
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
	std::cout << "handleClientRead" << std::endl;
	std::cout << "ClientRead() for fd: " << clientFd << std::endl;
	char buffer[4096];
	ssize_t bytes = recv(clientFd, buffer, sizeof(buffer), 0);
	struct epoll_event	epEvent;

	if (bytes <= 0) // Connection closed or error
	{
		std::cout << "Connection closed or error fd: " << clientFd << std::endl;
		close(clientFd);
		// TODO: is it necessary to remove the clientFd from the interest list of epoll?
		clients.erase(clientFd);  // Clean up client data
		return;
	}
	buffer[bytes] = '\0';
	std::cout << "Received " << bytes << " bytes from " << clientFd << std::endl;
    
	// Check if request pointer is valid
	if (clients[clientFd].request == NULL)
	{
		std::cerr << "ERROR: request pointer is NULL!" << std::endl;
		close(clientFd);
		clients.erase(clientFd);
		return;
	}

	// Parse request
	clients[clientFd].request->parseRequest(buffer);
	clients[clientFd].state = PROCESSING;
	// Build response (store in ClientConnection, not local variable!)
	clients[clientFd].response = new HttpResponse(clients[clientFd].request);
	clients[clientFd].state = SENDING_RESPONSE;
    
	// Switch to POLLOUT to send response
	// fds[index].events = POLLOUT;
	epEvent.events = EPOLLIN | EPOLLOUT;
	epEvent.data.fd = clientFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
		throw std::runtime_error(std::strerror(errno));
}

void	Server::handleClientWrite(int clientFd)
{
	std::cout << "handleClientWrite" << std::endl;
	ssize_t		sent, totalResponseSize;
	ClientConnection	&client = clients.at(clientFd);
	struct epoll_event	epEvent;

	if (!client.response)
		return ;

	std::string status_line = client.response->getStatusLine();
	sent = send(clientFd, status_line.c_str(), status_line.size(), 0);
	std::cout << "\033[36m____________________\nRESPONSE sending...\n\033[35m" << status_line << std::endl;

	std::string message_headers = client.response->getMessageHeaders();
	sent += send(clientFd, message_headers.c_str(), message_headers.size(), 0);
	std::cout << message_headers << std::endl;

	std::string message_body = client.response->getMessageBody();
	sent += send(clientFd, message_body.c_str(), message_body.size(), 0);
	std::cout << message_body << "____________________\033[m"<< std::endl;
	// std::cout << "bytest sent" << sent << std::endl;
	if (sent < 0)
		perror("send");
	totalResponseSize = status_line.size() + message_headers.size() + message_body.size();
	std::cout << "send bytes = " << sent << " totalResponseSize = " << totalResponseSize << std::endl;
	if (sent == totalResponseSize) {
		epEvent.events = EPOLLIN;
		epEvent.data.fd = clientFd;
		if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
			throw std::runtime_error(std::strerror(errno));
	}
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

	while (sigFlag != SIGINT)//true)
	{
		if ((nReady = epoll_wait(this->epollFd, ev, n, -1)) == -1)
			throw std::runtime_error(std::strerror(errno));
		for (int	i = 0; i < nReady; ++i) {
			if (this->isListenSock(ev[i].data.fd))
				this->handleNewClient(ev[i].data.fd);
			else if (ev[i].events & EPOLLIN) // the client is available for read
				this->handleClientRead(ev[i].data.fd);
			else if (ev[i].events & EPOLLOUT) // the client is available for write
				this->handleClientWrite(ev[i].data.fd);
		}
	}
}
