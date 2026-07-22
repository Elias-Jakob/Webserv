# include "Server.hpp"

void	Server::writeRequestBodyToCGI(int fd)
{
	ClientConnection	&caller = this->cgiPipes.at(fd);
	ssize_t	writtenBytes;
	std::string	requestBody = caller.request->getRequestBody();
	
	if (!caller.request->getRequestBody().empty()) {
		// TODO: handle the write (e.g. what should be done if the pipe is full and -1 is returned because the pipes fd is set to NONBLOCK)
		writtenBytes = write(fd, requestBody.c_str(), requestBody.size());
		if (writtenBytes == -1)
			return ;
			// throw std::runtime_error("failed while writing to cgi pipe: " + std::string(std::strerror(errno)));
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
	char buf[1024] = { 0 };
	ssize_t	readBytes;
	ClientConnection	&caller = this->cgiPipes.at(fd);

	readBytes = read(fd, buf, sizeof(buf) - 1);
	if (readBytes == -1) // handle
		return ;
		// throw std::runtime_error(std::strerror(errno));
	else if (readBytes > 0)
		caller.response_buffer.append(buf);
	else {
		if (caller.response_buffer.empty())
			caller.response_buffer = this->responseBuilder.errorResponseViaCode(500);
		close(fd);
		this->cgiPipes.erase(fd);
		caller.cgiOut = -1;
	}
}

void	Server::cgiTimeoutResponse(ClientConnection &client)
{
	client.terminateCGIProcess(&(this->cgiPipes));
	client.response_buffer = client.responseBuilder->errorResponseViaCode(504);
	client.state = SENDING_RESPONSE; // TODO: is setting the client state even necessary?
	this->epoll.ctl(client.fd, EPOLL_CTL_MOD, EPOLLOUT);
}

/**
	* @brief Check if any cgi process exited
*/
void	Server::awaitCGIProcesses()
{
	int status;

	for (std::map<int, ClientConnection>::iterator	it = this->clients.begin();
			it != this->clients.end(); ++it) {
		if (it->second.cgiPid == -1) continue ;
		if (waitpid(it->second.cgiPid, &status, WNOHANG) > 0) {
			if (WIFEXITED(status)) {
				it->second.response_buffer = (WEXITSTATUS(status) != 0) ?
					this->responseBuilder.errorResponseViaCode(500) :
					this->responseBuilder.cgiResponse(it->second.response_buffer, it->second.keep_alive);
			} else if (WIFSIGNALED(status))
				it->second.response_buffer = this->responseBuilder.errorResponseViaCode(500);
			it->second.state = SENDING_RESPONSE;
			this->epoll.ctl(it->second.fd, EPOLL_CTL_MOD, EPOLLOUT);
			it->second.cgiPid = -1;
		}
	}
}

