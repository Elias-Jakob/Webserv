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
	this->justRemovedFds.insert(caller.cgiIn);
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
		caller.response_buffer = (caller.response_buffer.empty())
			? this->responseBuilder.errorResponseViaCode(500)
			: this->responseBuilder.cgiResponse(caller.response_buffer, caller.keep_alive);
		close(caller.cgiOut);
		this->justRemovedFds.insert(caller.cgiOut);
		this->cgiPipes.erase(caller.cgiOut);
		caller.cgiOut = -1;
		// if (caller.cgiIn != -1) {
		// 	this->justRemovedFds.insert(caller.cgiIn);
		// 	close(caller.cgiIn);
		// 	this->cgiPipes.erase(caller.cgiIn);
		// 	caller.cgiIn = -1;
		// }
		caller.state = SENDING_RESPONSE;
		this->epoll.ctl(caller.fd, EPOLL_CTL_MOD, EPOLLOUT);
	}
}

void	Server::cgiTimeoutResponse(ClientConnection &client)
{
	this->justRemovedFds.insert(client.cgiIn);
	this->justRemovedFds.insert(client.cgiOut);
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
	int status, error = 0;
	pid_t	pid = waitpid(client.cgiPid, &status, WNOHANG);

	if (pid == 0)
		return ;
	if (pid > 0) {
		if (!WIFEXITED(status) && !WIFSIGNALED(status))
			return ;
		// if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		// 	this->responseBuilder.cgiResponse(client.response_buffer, client.keep_alive);
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			error = 500;
	}
	else {
		std::cerr << "Error: waitpid failed: " << std::strerror(errno) << std::endl;
		error = 500;
	}
	client.cgiPid = -1;
	if (error) {
		// if (client.cgiIn != -1) {
		// 	this->justRemovedFds.insert(client.cgiIn);
		// 	close(client.cgiIn);
		// 	this->cgiPipes.erase(client.cgiIn);
		// 	client.cgiIn = -1;
		// }
		// if (client.cgiOut != -1) {
		// 	this->justRemovedFds.insert(client.cgiOut);
		// 	close(client.cgiOut);
		// 	this->cgiPipes.erase(client.cgiOut);
		// 	client.cgiOut = -1;
		// }
		client.response_buffer = this->responseBuilder.errorResponseViaCode(500);
		client.state = SENDING_RESPONSE;
		this->epoll.ctl(client.fd, EPOLL_CTL_MOD, EPOLLOUT);
	}
}
