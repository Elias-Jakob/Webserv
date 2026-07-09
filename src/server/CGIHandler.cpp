# include "Server.hpp"

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

void	Server::handleCGIOutput(int fd)
{
	struct epoll_event	epEvent;
	char buf[1024] = { 0 };
	ssize_t	readBytes;
	int status;
	t_CGIProcess	&cgiProc = this->cgiProcesses.at(fd);
	ClientConnection	&client = *(cgiProc.client);

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
		waitpid(cgiProc.pid, &status, WNOHANG);
		// TODO: What should happen if the exit status of cgi process indicates failure?
		cgiProcesses.erase(fd);
		close(fd);
	} else {
		client.response_buffer.append(buf);
	}
}


