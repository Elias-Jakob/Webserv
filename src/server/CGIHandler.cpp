# include "Server.hpp"

void	Server::writeRequestBodyToCGI(int fd)
{
	ClientConnection	&caller = this->cgiPipes.at(fd);
	ssize_t	writtenBytes;
	
	if (!caller.request->getRequestBody().empty()) {
		// TODO: important; set cgiIn to nonblocking
		writtenBytes = write(fd, caller.request->getRequestBody().c_str(), caller.request->getRequestBody().size());
		// TODO: measure how many bytes of the request body were sent and how many still need to be sent
	}
	close(fd);
	this->cgiPipes.erase(fd);
	caller.cgiIn = -1;
}

void	Server::handleCGIOutput(int fd)
{
	struct epoll_event	epEvent;
	char buf[1024] = { 0 };
	ssize_t	readBytes;
	int status;
	ClientConnection	&caller = this->cgiPipes.at(fd);

	readBytes = read(fd, buf, sizeof(buf) - 1);
	std::string	strBuf(buf);
	if (readBytes == -1) // handle
		throw std::runtime_error(std::strerror(errno));
	if (readBytes == 0) { // pipe was closed
		caller.response_buffer = this->responseBuilder.cgiResponse(caller.response_buffer);
		caller.state = SENDING_RESPONSE;
		epEvent.events = EPOLLOUT;
		epEvent.data.fd = caller.fd;
		if (epoll_ctl(epollFd, EPOLL_CTL_MOD, epEvent.data.fd, &epEvent) == -1)
			throw std::runtime_error(std::strerror(errno));
		waitpid(caller.cgiPid, &status, WNOHANG);
		// TODO: What should happen if the exit status of cgi process indicates failure?
		close(fd);
		this->cgiPipes.erase(fd);
		caller.cgiPid = caller.cgiOut = -1;
	} else {
		caller.response_buffer.append(buf);
	}
}


