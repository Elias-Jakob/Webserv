# include "Server.hpp"

void	Server::handleIncoming(ClientConnection &caller)
{
	ssize_t	bytesRecv;
	char		buffer[RECV_BUFFER_SIZE] = { 0 };

	if (caller.state != IDLE && caller.state != READING_REQUEST)
		return ;
	std::cout << "ClientRead() for: " << caller.remoteAddr << std::endl;
	caller.state = READING_REQUEST;
	bytesRecv = recv(caller.fd, buffer, sizeof(buffer), 0);
	if (bytesRecv == -1)
		return ;
	else if (bytesRecv == 0)
	{
		std::cout << "Client closed connection" << std::endl;
		this->removeClient(caller);
		return ;
	}
	std::cout << "Received " << bytesRecv << " bytes from " << caller.remoteAddr << std::endl;
	caller.request->parseRequest(std::string(buffer, bytesRecv), bytesRecv);
	if (caller.request->parsingComplete()) {
		caller.state = PROCESSING;
		caller.processRequest();
		if (caller.state == CGI_PROCESSING) {
			try {
				if (caller.cgiPid != -1 || caller.cgiIn != -1 || caller.cgiOut != -1) {
					this->terminateClientCGI(caller);
					std::cout << "Re-request CGI: Interupting/Terminating previouse CGI process" << std::endl;
				}
				this->cgiLauncher.newProcess(caller);
			}
			catch (const std::exception &e) {
				this->terminateClientCGI(caller);
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

