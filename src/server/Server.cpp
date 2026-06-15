# include "Server.hpp"

Server::Server(const char *interface, const char *port) :
	interface(interface), port(port)
{

}

Server::~Server()
{

}

void	Server::serverStartup()
{
	struct addrinfo	hints;

	this->epollFd = epoll_create1(0);
	if (this->epollFd == -1)
		throw std::runtime_error(std::strerror(errno));
	// INFO: hints indicate what the returned socket address will be used for
	std::memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC; // dont care if a IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // we want a stable connection base socket
	hints.ai_protocol = IPPROTO_TCP; // redundant: by chosing SOCK_STREAM as socktype the protocol would defualt to TCP
	hints.ai_flags = AI_PASSIVE; // indicates that the returned socket address structure is intended for use in a call to bind
	// TODO: replace with loop through all interface:port pairs
	Socket	sock(this->epollFD, hints, this->interface, this->port);
	this->sockets.push_back(sock);
	
}
