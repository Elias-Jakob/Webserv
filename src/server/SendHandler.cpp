# include "Server.hpp"

void	Server::handleOutgoing(int clientFd)
{
	ClientConnection	&client = this->clients.at(clientFd);
	ssize_t		sentBytes;

	std::cout << "\033[35m==========\nRESPONSE sending...\n" << std::endl;
	sentBytes = send(clientFd, client.response_buffer.c_str(), client.response_buffer.size(), 0);
	if (sentBytes == -1)
		return ;
		// throw std::runtime_error("send failed: " + std::string(std::strerror(errno))); // TODO:
	client.bytesSent += sentBytes;
	if (client.bytesSent < client.response_buffer.size()) {
		std::cout << "bytes sent: " << sentBytes << " response_buffer: " << client.response_buffer.size() << " was not fully sent" << std::endl;
		return;
	}
	if (PRINT_RESPONSE)
		std::cout << client.response_buffer << std::endl;
	std::cout << "bytes sent: " << sentBytes << "\n==========\033[m" << std::endl;
	if (!client.keep_alive || client.timeout)
	{
		std::cout << ((client.timeout) ? "Timeout: close client connection" : "Client connection is not set to keep-alive, closing socket...") << std::endl;
		this->removeClient(client);
		return;
	}
	client.cleanUpClient();
	this->epoll.ctl(clientFd, EPOLL_CTL_MOD, EPOLLIN);
}


