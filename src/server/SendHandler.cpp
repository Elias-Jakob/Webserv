# include "Server.hpp"

void	Server::handleOutgoing(ClientConnection &caller)
{
	ssize_t	sentBytes;

	std::cout << "Send gets called here" << std::endl;
	sentBytes = send(caller.fd, caller.response_buffer.c_str() + caller.bytesSent,
		caller.response_buffer.size(), 0);
	if (sentBytes == -1)
		return ;
	caller.bytesSent += sentBytes;
	if (caller.bytesSent < caller.response_buffer.size())
		return;
	if (PRINT_RESPONSE)
		std::cout << "Response (size = " << caller.response_buffer.size() << "): "
			<< caller.response_buffer << std::endl;
	if (!caller.keep_alive || caller.timeout)
	{
		std::cout << ((caller.timeout) ? "Timeout: close caller connection" :
			"Client connection is not set to keep-alive, closing socket...") << std::endl;
		this->removeClient(caller);
		return;
	}
	caller.cleanUpClient();
	this->epoll.ctl(caller.fd, EPOLL_CTL_MOD, EPOLLIN);
}
