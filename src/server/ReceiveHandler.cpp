# include "Server.hpp"

void	Server::handleIncoming(int clientFd)
{
	ClientConnection	&client = this->clients.at(clientFd);
	ssize_t	bytesRecv;
	char buffer[4096];
	// struct epoll_event	epEvent;

	std::cout << "ClientRead() for fd: " << clientFd << std::endl;
	bytesRecv = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRecv == -1)
		throw std::runtime_error(std::strerror(errno));
	else if (bytesRecv == 0)
	{
		// Client closed connection
		std::cout << "Client closed connection" << std::endl;
		this->removeClient(client);
		return;
	}
	std::cout << "Received " << bytesRecv << " bytes from " << clientFd << std::endl;
	if (client.request == NULL)// Check if request pointer is valid
	{
		std::cerr << "ERROR: request pointer is NULL!" << std::endl;
		this->removeClient(client);
		return;
	}
	// Parse request - use string constructor with length to avoid buffer overflow
	std::string request_data(buffer, bytesRecv);
	client.request->parseRequest(request_data);
	if (client.request->parsingComplete())
	{
		client.state = PROCESSING;
		client.processRequest();
		if (client.state == CGI_PROCESSING) {
			try {
				if (client.cgiPid != -1) {
					client.terminateCGIProcess(&(this->cgiPipes));
					std::cout << "Re-request CGI: Interupting/Terminating previouse CGI process" << std::endl;
				}
				this->cgiLauncher.newProcess(client);
			}
			catch (const CGIError &e) {
				std::cerr << "CGI Process failed: " << e.what() << std::endl;
			}
		}
		else {
			client.state = SENDING_RESPONSE;
			this->epoll.ctl(clientFd, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
			// epEvent.events = EPOLLIN | EPOLLOUT;
			// epEvent.data.fd = clientFd;
			// if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
			// 	throw std::runtime_error(std::strerror(errno));
		}
	}
}

