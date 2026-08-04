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
void	Server::acceptNewClient(int listenFd)
{
	int	fd;
	struct sockaddr	clientAddr;
	socklen_t	clientAddrLen = sizeof(clientAddr);
	ClientConnection	*client = NULL;

	try {
		fd = accept(listenFd, &clientAddr, &clientAddrLen);
		if (fd == -1)
			throw std::runtime_error(std::strerror(errno));
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1 || fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
			throw std::runtime_error(std::strerror(errno));
		client = &(this->clients[fd]);
		this->epoll.ctl(fd, EPOLL_CTL_ADD, EPOLLIN);
		client->fd = fd;
		client->request = new HttpRequest();
		client->executor = &(this->methodExecuter);
		client->responseBuilder = &(this->responseBuilder);
		client->sessionManager = &(this->sessionManager);
		client->bytesSent = 0;
		client->_listeningInterface = listenFdToInterface[listenFd];
		client->request->setServerConfigs(client->executor->getServerConfigs(),
			this->listenFdToInterface[listenFd]);
		client->remoteAddr = utils::addrToStr(clientAddr);
		std::cout << "New client " << client->fd << " " << client->remoteAddr
			<< " connected over socket " << listenFdToInterface[listenFd] << std::endl;
	}
	catch (const std::runtime_error	&e) {
		if (client) this->removeClient(*client);
		else if (fd != -1) close(fd);
		std::cerr << "Failed to connect new client over socket "
			<< listenFdToInterface[listenFd] << ": " << e.what() << std::endl;
	}
}

void	Server::removeClient(ClientConnection &client)
{
	std::cout << "removeClient: " << client.fd << " " << client.remoteAddr << std::endl;
	this->justRemovedFds.insert(client.fd);
	this->terminateClientCGI(client);
	close(client.fd);
	this->clients.erase(client.fd);
}

ClientConnection	*Server::identifyEventCaller(int fd)
{
	std::map<int, ClientConnection>::iterator	caller = this->clients.find(fd);
	if (caller != this->clients.end())
		return (&caller->second);
	std::map<int, int>::iterator	isPipe = this->cgiPipes.find(fd);
	if (isPipe == this->cgiPipes.end())
		return (NULL);
	caller = this->clients.find(isPipe->second);
	if (caller != this->clients.end())
		return (&caller->second);
	return (NULL);
}

void	Server::callEventHandler(const struct epoll_event &event)
{
	ClientConnection	*caller = this->identifyEventCaller(event.data.fd);

	if (caller == NULL) return ;
	try {
		if (event.events & EPOLLERR)
			throw std::runtime_error("Error condition happened on the associated file descriptor");
		if (event.data.fd == caller->fd) {
			caller->inactiveTime = std::time(NULL);
			if (event.events & EPOLLIN)
				this->handleIncoming(*caller);
			if (event.events & EPOLLOUT)
				this->handleOutgoing(*caller);
		}
		else if (event.data.fd == caller->cgiIn && event.events & EPOLLOUT)
			this->writeRequestBodyToCGI(*caller);
		else if (event.data.fd == caller->cgiOut && event.events & (EPOLLIN | EPOLLHUP))
			this->handleCGIOutput(*caller);
	} catch (const std::runtime_error &e) {
		std::cerr << "Error in Server::callEventHandler: " << e.what() << "\nRemoving client "
			<< caller->remoteAddr << std::endl;
		std::cout << "Coming from callEventHandler catch..." << std::endl;
		this->removeClient(*caller);
	}
}

void	Server::checkOnClients()
{
	std::map<int, ClientConnection>::iterator	it = this->clients.begin();
	time_t	current = std::time(NULL);

	while (it != this->clients.end()) {
		try {
			if (it->second.cgiPid != -1)
				this->checkProcessStatus(it->second);
			// client timeout
			if (current - it->second.inactiveTime >= KEEP_ALIVE_TIMEOUT && !it->second.timeout) {
				std::cout << "Removed inactive client " << it->second.remoteAddr
					<< " after timeout of " << current - it->second.inactiveTime << std::endl;
				it->second.timeout = true;
				if (it->second.cgiPid == -1) {
					if (it->second.state == IDLE) {
						this->removeClient((it++)->second);
						continue ;
					}
					it->second.response_buffer = it->second.responseBuilder->errorResponseViaCode(400);
					it->second.state = SENDING_RESPONSE;
					this->epoll.ctl(it->second.fd, EPOLL_CTL_MOD, EPOLLOUT);
				} else
					this->cgiTimeoutResponse(it->second);
			}
			// cgi timeout
			else if (it->second.cgiPid != -1 && current - it->second.cgiStartTime >= CGI_TIMEOUT)
				this->cgiTimeoutResponse(it->second);
			it++;
		} catch (const std::runtime_error &e) {
			std::cerr << "Error in Server::checkOnClients: " << e.what() << "\nRemoving client "
				<< it->second.remoteAddr << std::endl;
			this->removeClient((it++)->second);
		}
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
			if (this->justRemovedFds.count(events[i].data.fd))
				continue;
			if (std::find(this->listenSockets.begin(), this->listenSockets.end(),
					events[i].data.fd) != this->listenSockets.end()) {
				this->acceptNewClient(events[i].data.fd);
				continue;
			}
			this->callEventHandler(events[i]);
		}
		this->checkOnClients();
		this->sessionManager.removeExpiredSessions();
		this->justRemovedFds.clear();
	}
}
