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
	struct sockaddr	clientAddr;
	socklen_t	clientAddrLen = sizeof(clientAddr);

	fd = accept(listenFd, &clientAddr, &clientAddrLen);
	if (fd == -1)
		throw std::runtime_error(std::strerror(errno));
	this->clients[fd].remoteAddr = utils::addrToStr(clientAddr);
	std::cout << "New client connected... socket file descriptor = " << fd << " ip: " << this->clients[fd].remoteAddr << std::endl;
	// TODO: remove F_SETFD FD_CLOEXEC?
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1 || fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
		throw std::runtime_error(std::strerror(errno));

	// Initialize client connection directly in map (avoid copy issues)
	//this->clients[fd] = ClientConnection();
	this->clients[fd].fd = fd;
	this->clients[fd].state = READING_REQUEST;
	this->clients[fd].request = new HttpRequest();
	this->clients[fd].executor = &(this->methodExecuter);
	this->clients[fd].responseBuilder = &(this->responseBuilder);
	this->clients[fd].bytesSent = 0;
	this->clients[fd]._listeningInterface = listenFdToInterface[listenFd];
	this->clients[fd].request->setServerConfigs(clients[fd].executor->getServerConfigs(), listenFdToInterface[listenFd]);
	// TODO: clean up
	// this->clients[fd] = ClientConnection(fd);
	this->epoll.ctl(fd, EPOLL_CTL_ADD, EPOLLIN);
}

void	Server::removeClient(ClientConnection &client)
{
	if (client.cgiPid != -1)
		client.terminateCGIProcess(&(this->cgiPipes));
	close(client.fd);
	this->clients.erase(client.fd);
}

void	Server::checkTimeouts()
{
	std::map<int, ClientConnection>::iterator	it = this->clients.begin();
	time_t	current = std::time(NULL);

	while (it != this->clients.end()) {
		// client timeout
		if (current - it->second.inactiveTime >= KEEP_ALIVE_TIMEOUT && !it->second.timeout) {
			std::cout << "Removed inactive client after timeout... fd = " << it->first << " inactiveTime = " << current - it->second.inactiveTime << std::endl;
			it->second.timeout = true;
			if (it->second.cgiPid == -1) {
				this->removeClient((it++)->second);
				continue ;
			}
			this->cgiTimeoutResponse(it->second);
		}
		// cgi timeout
		else if (it->second.cgiPid != -1 && current - it->second.cgiStartTime >= CGI_TIMEOUT) {
			std::cout << "Terminating CGI process after timeout... client fd = " << it->first << std::endl;
			this->cgiTimeoutResponse(it->second);
		}
		it++;
	}
}

void	Server::eventLoop()
{
	int	nFds;
	struct epoll_event	events[EPOLL_MAX_EVENTS];

	while (sigFlag != SIGINT)
	{
		nFds = epoll_wait(this->epoll.fd, events, EPOLL_MAX_EVENTS, 1000);
		if (nFds == -1) {
			if (sigFlag == SIGINT)
				break ;
			throw std::runtime_error(std::strerror(errno));
		}
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
				this->clients.at(events[i].data.fd).inactiveTime = std::time(NULL);
				if (events[i].events & EPOLLIN) // the client is available for read
					this->handleIncoming(events[i].data.fd);
				if (events[i].events & EPOLLOUT) // the client is available for write
					this->handleOutgoing(events[i].data.fd);
			} else if (this->cgiPipes.find(events[i].data.fd) != this->cgiPipes.end()) {
				if (events[i].events & EPOLLOUT)
					this->writeRequestBodyToCGI(events[i].data.fd);
				else// if (events[i].events & EPOLLIN)
					this->handleCGIOutput(events[i].data.fd);
			}
		}
		this->checkTimeouts();
		// TODO: refactor integrate awaitCGIProcesses into checkTimeouts
		this->awaitCGIProcesses();
	}
}
