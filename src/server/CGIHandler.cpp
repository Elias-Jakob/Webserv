# include "Server.hpp"

void	Server::writeRequestBodyToCGI(ClientConnection &caller)
{
	ssize_t	writtenBytes;
	std::string	requestBody = caller.request->getRequestBody();
	
	if (caller.state != CGI_PROCESSING)
		return ;
	if (!caller.request->getRequestBody().empty()) {
		writtenBytes = write(caller.cgiIn, requestBody.c_str() + caller.cgiWrittenBytes,
			requestBody.size() - caller.cgiWrittenBytes);
		if (writtenBytes == -1)
			return ;
		caller.cgiWrittenBytes += writtenBytes;
		if (caller.cgiWrittenBytes < requestBody.size())
			return ;
	}
	close(caller.cgiIn);
	this->justRemovedFds.insert(caller.cgiIn);
	this->cgiPipes.erase(caller.cgiIn);
	caller.cgiIn = -1;
}

void	Server::handleCGIOutput(ClientConnection &caller)
{
	char		buffer[RECV_BUFFER_SIZE] = { 0 };
	ssize_t	readBytes;

	if (caller.state != CGI_PROCESSING)
		return ;
	readBytes = read(caller.cgiOut, buffer, sizeof(buffer));
	if (readBytes == -1)
		return ;
	else if (readBytes > 0)
		caller.response_buffer.append(buffer, readBytes);
	else {
		if (caller.response_buffer.empty())
			caller.response_buffer = this->responseBuilder.errorResponseViaCode(500);
		else {
			caller.applyCgiSessionHeaders();
			if (caller.sendCookie)
				caller.response_buffer = this->responseBuilder.cgiResponse(caller.response_buffer, caller.keep_alive, caller.cookieHeader);
			else
				caller.response_buffer = this->responseBuilder.cgiResponse(caller.response_buffer, caller.keep_alive);
		}
		close(caller.cgiOut);
		this->justRemovedFds.insert(caller.cgiOut);
		this->cgiPipes.erase(caller.cgiOut);
		caller.cgiOut = -1;
		caller.state = SENDING_RESPONSE;
		this->epoll.ctl(caller.fd, EPOLL_CTL_MOD, EPOLLOUT);
	}
}

void	Server::terminateClientCGI(ClientConnection &client)
{
	int	status;
	
	if (client.cgiPid != -1) {
		kill(client.cgiPid, SIGKILL);
		waitpid(client.cgiPid, &status, 0);
	}
	if (client.cgiIn != -1) {
		this->justRemovedFds.insert(client.cgiIn);
		close(client.cgiIn);
		this->cgiPipes.erase(client.cgiIn);
	}
	if (client.cgiOut != -1) {
		this->justRemovedFds.insert(client.cgiOut);
		close(client.cgiOut);
		this->cgiPipes.erase(client.cgiOut);
	}
	client.cgiPid = client.cgiIn = client.cgiOut = -1;
	client.cgiWrittenBytes = 0;
}

void	Server::cgiTimeoutResponse(ClientConnection &client)
{
	this->terminateClientCGI(client);
	client.response_buffer = client.responseBuilder->errorResponseViaCode(504);
	client.state = SENDING_RESPONSE;
	this->epoll.ctl(client.fd, EPOLL_CTL_MOD, EPOLLOUT);
}

/**
	* @brief Check if cgi process terminated
*/
void	Server::checkProcessStatus(ClientConnection &client)
{
	int status, error = 0;
	pid_t	pid = waitpid(client.cgiPid, &status, WNOHANG);

	if (pid == 0)
		return ;
	if (pid > 0) {
		if (!WIFEXITED(status) && !WIFSIGNALED(status))
			return ;
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			error = 500;
	}
	else {
		std::cerr << "Error: waitpid failed: " << std::strerror(errno) << std::endl;
		error = 500;
	}
	client.cgiPid = -1;
	if (error) {
		this->terminateClientCGI(client);
		client.response_buffer = this->responseBuilder.errorResponseViaCode(500);
		client.state = SENDING_RESPONSE;
		this->epoll.ctl(client.fd, EPOLL_CTL_MOD, EPOLLOUT);
	}
}
