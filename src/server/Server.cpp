# include "Server.hpp"

Server::Server(const char *interface, const char *port) :
	epollFd(-1), interface(interface), port(port)
{}

Server::~Server()
{
	if (this->epollFd != -1)
		close(this->epollFd);
}

bool	Server::listenSock(int readyFd)
{
	// Check only the nListeningSockets
	for (std::vector<Socket>::iterator	it = this->sockets.begin();
				it != this->sockets.end(); ++it) {
		if (!(*(it).isListenSock())) break;
		if ((*it).getFd() == readyFd)
			return (true);
	}
	return (false);
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
	
	int	n = 1;
	struct epoll_event	ready_clients[n];
	int	efd;

	while (true)
	{
		if (wait_epoll(this->epollFd, ev, n, 10000) == -1)
			throw std::runtime_error(std::strerror(errno));
		if (this->listenSock(ev[0]))
			; // -> add client socket to sockets
		else if () // check for read or write
	}
}
