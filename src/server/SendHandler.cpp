# include "Server.hpp"

void	Server::handleOutgoing(int clientFd)
{
	ClientConnection	&client = this->clients.at(clientFd);
	ssize_t		sentBytes;
	struct epoll_event	epEvent;

	std::cout << "\033[35m==========\nRESPONSE sending...\n" << std::endl;
	sentBytes = send(clientFd, client.response_buffer.c_str(), client.response_buffer.size(), 0);
	if (sentBytes == -1)
		throw std::runtime_error("send failed: " + std::string(std::strerror(errno))); // TODO:
	client.bytesSent += sentBytes;
	if (client.bytesSent < client.response_buffer.size()) {
		std::cout << "bytes sent: " << sentBytes << " response_buffer: " << client.response_buffer.size() << " was not fully sent" << std::endl;
		return;
	}
	if (PRINT_RESPONSE)
		std::cout << client.response_buffer << std::endl;
	std::cout << "bytes sent: " << sentBytes << "\n==========\033[m" << std::endl;
	// std::cout << "response_buffer: " << client.response_buffer << std::endl;
	// Clean up: close connection and remove from tracking
	if (client.keep_alive == false)
	{
		std::cout << "Client connection is not set to keep-alive, closing socket..." << std::endl;
		this->removeClient(client);
		return;
	}
	epEvent.events = EPOLLIN;
	epEvent.data.fd = clientFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &epEvent) == -1)
		throw std::runtime_error(std::strerror(errno));
	client.cleanUpClient();
}


