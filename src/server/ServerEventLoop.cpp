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
	ClientConnection	&client = this->clients.at(clientFd);
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
		this->removeClient(client);
		return;
	}
	std::cout << "Received " << bytesRecv << " bytes from " << clientFd << std::endl;
	if (client.request == NULL)// Check if request pointer is valid
	{
		std::cerr << "ERROR: request pointer is NULL!" << std::endl;
		this->removeClient(client);
		return;
	}
	// Parse request - use string constructor with length to avoid buffer overflow
	std::string request_data(buffer, bytesRecv);
	client.request->parseRequest(request_data);
	if (client.request->parsingComplete())
	{
		client.state = PROCESSING;
		client.processRequest();
		if (client.state == CGI_PROCESSING) {
			try {
				this->cgiLauncher.newProcess(client);
			}
			catch (const CGIError &e) {
				std::cerr << "CGI Process failed: " << e.what() << std::endl;
			}
		}
		else {
			client.state = SENDING_RESPONSE;
			epEvent.events = EPOLLIN | EPOLLOUT;
			epEvent.data.fd = clientFd;
			if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
				throw std::runtime_error(std::strerror(errno));
		}
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
	if (PRINT_RESPONSE)
		std::cout << client.response_buffer << std::endl;
	std::cout << "bytes sent: " << sentBytes << "\n==========\033[m" << std::endl;
	// std::cout << "response_buffer: " << client.response_buffer << std::endl;
	// Clean up: close connection and remove from tracking
	if (client.keep_alive == false)
	{
		std::cout << "Client connection is not set to keep-alive, closing socket..." << std::endl;
		this->removeClient(client);
		return;
	}
	epEvent.events = EPOLLIN;
	epEvent.data.fd = clientFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
		throw std::runtime_error(std::strerror(errno));
	client.cleanUpClient();
}

void	Server::killCGIProcesses(ClientConnection &client)
{
	struct epoll_event	epEvent;
	
	for (std::map<int, t_CGIProcess>::iterator	it = this->cgiProcesses.begin();
			it != this->cgiProcesses.end(); ) {
		if (it->second.client == &client) {
			kill(it->second.pid, SIGKILL);
			epEvent.data.fd = it->first;
			if (epoll_ctl(epollFd, EPOLL_CTL_DEL, it->first, NULL) == -1)
				throw std::runtime_error(std::strerror(errno));
			close(it->first);
			this->cgiProcesses.erase(it++);
		} else
			++it;
	}
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

void	Server::handleCGIWrite(int fd)
{
	struct epoll_event	epEvent;
	char buf[1024] = { 0 };
	int	readBytes;
	ClientConnection	&client = *this->cgiProcesses.at(fd).client;

	readBytes = read(fd, buf, sizeof(buf) - 1);
	std::string	strBuf(buf);
	if (readBytes == -1) // handle
		throw std::runtime_error(std::strerror(errno));
	if (readBytes == 0) { // pipe was closed
		
		client.response_buffer = this->responseBuilder.cgiFormation(client.response_buffer);
		client.state = SENDING_RESPONSE;
		epEvent.events = EPOLLOUT;
		epEvent.data.fd = client.fd;
		if (epoll_ctl(epollFd, EPOLL_CTL_MOD, epEvent.data.fd, &epEvent) == -1)
			throw std::runtime_error(std::strerror(errno));
		cgiProcesses.erase(fd);
		close(fd);
	} else {
		client.response_buffer.append(buf);
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
			if (this->clients.find(events[i].data.fd) != this->clients.end()) {
				if (events[i].events & (EPOLLHUP | EPOLLERR)) { // client was disconnected
					this->removeClient(this->clients.at(events[i].data.fd));
					continue;
				}
				this->clients.at(events[i].data.fd).inactiveTime = 0;
				if (events[i].events & EPOLLIN) // the client is available for read
					this->handleClientRead(events[i].data.fd);
				if (events[i].events & EPOLLOUT) // the client is available for write
					this->handleClientWrite(events[i].data.fd);
			} else if (this->cgiProcesses.find(events[i].data.fd) != this->cgiProcesses.end()) {
				handleCGIWrite(events[i].data.fd);
			}
		}
		this->timeoutInactiveClients();
	}
}
