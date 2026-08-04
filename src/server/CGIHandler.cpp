# include "Server.hpp"

void	Server::writeRequestBodyToCGI(ClientConnection &caller)
{
	ssize_t	writtenBytes;
	std::string	requestBody = caller.request->getRequestBody();
	
	if (!caller.request->getRequestBody().empty()) {
		writtenBytes = write(caller.cgiIn, requestBody.c_str(), requestBody.size());
		if (writtenBytes == -1)
			return ;
		caller.cgiWrittenBytes += writtenBytes;
		if (caller.cgiWrittenBytes < requestBody.size())
			return ;
	}
	close(caller.cgiIn);
	this->cgiPipes.erase(caller.cgiIn);
	caller.cgiIn = -1;
}

void	Server::handleCGIOutput(ClientConnection &caller)
{
	char buf[1024] = { 0 };
	ssize_t	readBytes;

	readBytes = read(caller.cgiOut, buf, sizeof(buf) - 1);
	if (readBytes == -1)
		return ;
	else if (readBytes > 0) {
		// std::cout << RED << "CGI_OUTPUT:\n" << buf << RESET << std::endl; // debug
		// std::cout << GREEN << "caller.buffer:\n" << caller.response_buffer << RESET << std::endl;
		caller.response_buffer.append(buf);
	}
	else {
		if (caller.response_buffer.empty())
			caller.response_buffer = this->responseBuilder.errorResponseViaCode(500);
		close(caller.cgiOut);
		this->cgiPipes.erase(caller.cgiOut);
		caller.cgiOut = -1;
	}
}

void	Server::cgiTimeoutResponse(ClientConnection &client)
{
	client.terminateCGIProcess(&(this->cgiPipes));
	client.response_buffer = client.responseBuilder->errorResponseViaCode(504);
	client.state = SENDING_RESPONSE;
	this->epoll.ctl(client.fd, EPOLL_CTL_MOD, EPOLLOUT);
}

/**
	* @brief Check if cgi process exited
*/
void	Server::checkProcessStatus(ClientConnection &client)
{
	int status;
	pid_t	pid = waitpid(client.cgiPid, &status, WNOHANG);

	if (pid == 0)
		return ;
	if (pid > 0) {
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) != 0)
				client.response_buffer = this->responseBuilder.errorResponseViaCode(500);
			else {
				client.applyCgiSessionHeaders();
				if (client.sendCookie)
					client.response_buffer = this->responseBuilder.cgiResponse(client.response_buffer, client.keep_alive, client.cookieHeader);
				else
					client.response_buffer = this->responseBuilder.cgiResponse(client.response_buffer, client.keep_alive);
			}
		} else if (WIFSIGNALED(status))
			client.response_buffer = this->responseBuilder.errorResponseViaCode(500);
	}
	else {
		std::cerr << "Error: waitpid failed: " << std::strerror(errno) << std::endl;
		client.response_buffer = this->responseBuilder.errorResponseViaCode(500);
	}
	client.state = SENDING_RESPONSE;
	this->epoll.ctl(client.fd, EPOLL_CTL_MOD, EPOLLOUT);
	client.cgiPid = -1;
}
