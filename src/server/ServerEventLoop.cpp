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
	std::cout << "New client connected... socket file descriptor = " << fd << std::endl;
	// TODO: check if FD_CLOEXEC works on linux
	if (fcntl(fd, F_SETFL, O_NONBLOCK | FD_CLOEXEC) == -1)
		throw std::runtime_error(std::strerror(errno));

	// Initialize client connection directly in map (avoid copy issues)
	//this->clients[fd] = ClientConnection();
	this->clients[fd].fd = fd;
	this->clients[fd].state = READING_REQUEST;
	this->clients[fd].request = new HttpRequest();
	this->clients[fd].executor = &(this->methodExecuter);
	this->clients[fd].responseBuilder = &(this->responseBuilder);
	this->clients[fd].bytesSent = 0;

	// TODO: clean up
	// this->clients[fd] = ClientConnection(fd);
	epEvent.events = EPOLLIN;
	epEvent.data.fd = fd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &epEvent) == -1)
		throw std::runtime_error(std::strerror(errno));
}

void	Server::removeClient(ClientConnection &client)
{
	this->killCGIProcesses(client);
	close(client.fd);
	this->clients.erase(client.fd);
}

void	Server::timeoutInactiveClients()
{
	for (std::map<int, ClientConnection>::iterator	it = this->clients.begin();
			it != this->clients.end(); ) {
		it->second.inactiveTime++;
		if (it->second.inactiveTime >= KEEP_ALIVE_TIMEOUT) {
			std::cout << "Removed inactive client after timeout... fd = " << it->first << std::endl;
			this->removeClient((it++)->second);
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
				this->handleNewClient(events[i].data.fd);
				continue;
			}
			if (this->clients.find(events[i].data.fd) != this->clients.end()) {
				if (events[i].events & (EPOLLHUP | EPOLLERR)) { // client was disconnected
					this->removeClient(this->clients.at(events[i].data.fd));
					continue;
				}
				this->clients.at(events[i].data.fd).inactiveTime = 0;
				if (events[i].events & EPOLLIN) // the client is available for read
					this->handleIncoming(events[i].data.fd);
				if (events[i].events & EPOLLOUT) // the client is available for write
					this->handleOutgoing(events[i].data.fd);
			} else if (this->cgiProcesses.find(events[i].data.fd) != this->cgiProcesses.end()) {
				handleCGIOutput(events[i].data.fd);
			}
		}
		this->timeoutInactiveClients();
	}
}
