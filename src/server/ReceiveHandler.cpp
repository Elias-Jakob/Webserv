# include "Server.hpp"

void	Server::handleIncoming(ClientConnection &caller)
{
	ssize_t	bytesRecv;
	char	buffer[4096] = { 0 };

	if (caller.state != IDLE && caller.state != READING_REQUEST)
		return ;
	std::cout << "ClientRead() for: " << caller.remoteAddr << std::endl;
	caller.state = READING_REQUEST;
	bytesRecv = recv(caller.fd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRecv == -1)
		return ;
	else if (bytesRecv == 0)
	{
		std::cout << "Client closed connection" << std::endl;
		this->removeClient(caller);
		return;
	}
	std::cout << "Received " << bytesRecv << " bytes from " << caller.remoteAddr << std::endl;
	// WARNING: redundant?
	// if (caller.request == NULL)// Check if request pointer is valid
	// {
	// 	std::cerr << "ERROR: request pointer is NULL!" << std::endl;
	// 	this->removeClient(caller);
	// 	return;
	// }
	// Parse request - use string constructor with length to avoid buffer overflow
	caller.request->parseRequest(std::string(buffer), bytesRecv);
	if (caller.request->parsingComplete()) {
		caller.state = PROCESSING;
		caller.processRequest();
		if (caller.state == CGI_PROCESSING) {
			try {
				if (caller.cgiPid != -1) {
					caller.terminateCGIProcess(&(this->cgiPipes));
					std::cout << "Re-request CGI: Interupting/Terminating previouse CGI process" << std::endl;
				}
				this->cgiLauncher.newProcess(caller);
			}
			catch (const std::exception &e) {
				if (caller.cgiPid != -1)
					std::cerr << "ReceiveHandler caller.cgiPid: " << caller.cgiPid << std::endl;
				// 	caller.terminateCGIProcess(&(this->cgiPipes));
				caller.response_buffer = caller.responseBuilder->errorResponseViaCode(500);
				caller.state = SENDING_RESPONSE;
				this->epoll.ctl(caller.fd, EPOLL_CTL_MOD, EPOLLOUT);
				std::cerr << "CGI process failed: " << e.what() << std::endl;
			}
		}
		else {
			caller.state = SENDING_RESPONSE;
			this->epoll.ctl(caller.fd, EPOLL_CTL_MOD, EPOLLOUT);
		}
	}
}

