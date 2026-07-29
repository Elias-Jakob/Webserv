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
	else if (readBytes > 0)
		caller.response_buffer.append(buf);
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

	if (waitpid(client.cgiPid, &status, WNOHANG) > 0) {
		if (WIFEXITED(status)) {
			client.response_buffer = (WEXITSTATUS(status) != 0) ?
				this->responseBuilder.errorResponseViaCode(500) :
				this->responseBuilder.cgiResponse(client.response_buffer, client.keep_alive);
		} else if (WIFSIGNALED(status))
			client.response_buffer = this->responseBuilder.errorResponseViaCode(500);
		client.state = SENDING_RESPONSE;
		this->epoll.ctl(client.fd, EPOLL_CTL_MOD, EPOLLOUT);
		client.cgiPid = -1;
	}
}

