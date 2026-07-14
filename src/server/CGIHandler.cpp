# include "Server.hpp"

void	Server::writeRequestBodyToCGI(int fd)
{
	ClientConnection	&caller = this->cgiPipes.at(fd);
	ssize_t	writtenBytes;
	std::string	requestBody = caller.request->getRequestBody();
	
	if (!caller.request->getRequestBody().empty()) {
		writtenBytes = write(fd, requestBody.c_str(), requestBody.size());
		if (writtenBytes == -1)
			throw std::runtime_error("failed while writing to cgi pipe: " + std::string(std::strerror(errno)));
		caller.cgiWrittenBytes += writtenBytes;
		if (caller.cgiWrittenBytes < requestBody.size())
			return ;
	}
	close(fd);
	this->cgiPipes.erase(fd);
	caller.cgiIn = -1;
}

void	Server::handleCGIOutput(int fd)
{
	// struct epoll_event	epEvent;
	char buf[1024] = { 0 };
	ssize_t	readBytes;
	int status;
	ClientConnection	&caller = this->cgiPipes.at(fd);

	readBytes = read(fd, buf, sizeof(buf) - 1);
	std::string	strBuf(buf);
	if (readBytes == -1) // handle
		throw std::runtime_error(std::strerror(errno));
	if (readBytes == 0) { // pipe was closed
		if (caller.response_buffer.empty())
			caller.response_buffer = this->responseBuilder.buildErrorResponse(500);
		else
			caller.response_buffer = this->responseBuilder.cgiFormation(caller.response_buffer);
		caller.state = SENDING_RESPONSE;
		this->epoll.ctl(caller.fd, EPOLL_CTL_MOD, EPOLLOUT);
		// epEvent.events = EPOLLOUT;
		// epEvent.data.fd = caller.fd;
		// if (epoll_ctl(epollFd, EPOLL_CTL_MOD, epEvent.data.fd, &epEvent) == -1)
		// 	throw std::runtime_error(std::strerror(errno));
		waitpid(caller.cgiPid, &status, WNOHANG);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
			caller.response_buffer = this->responseBuilder.buildErrorResponse(500);
		// TODO: What should happen if the exit status of cgi process indicates failure?
		close(fd);
		this->cgiPipes.erase(fd);
		caller.cgiPid = caller.cgiOut = -1;
	} else {
		caller.response_buffer.append(buf);
	}
}


