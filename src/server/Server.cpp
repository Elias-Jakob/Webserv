# include "Server.hpp"

Server::Server(const char *interface, const char *port) :
	interface(interface), port(port), epollFd(-1)
{}

Server::~Server()
{
	std::cout << "Destructing server..." << std::endl;
	for (std::vector<int>::iterator	it = this->listenSockets.begin();
			it != this->listenSockets.end(); ++it)
		close(*it);
	for (std::map<int, ClientConnection>::iterator	it = this->clients.begin();
			it != this->clients.end(); ++it)
		close(it->first);
	if (this->epollFd != -1)
		close(this->epollFd);
}

void	Server::initListenSockets()
{
	struct addrinfo	hints, *res;
	int	gaiErr, fd;
	struct epoll_event	epEvent;

	// INFO: hints indicate what the returned socket address will be used for
	std::memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC; // dont care if a IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // we want a stable connection base socket
	hints.ai_protocol = IPPROTO_TCP; // redundant: by chosing SOCK_STREAM as socktype the protocol would defualt to TCP
	hints.ai_flags = AI_PASSIVE; // indicates that the returned socket address structure is intended for use in a call to bind

	// ?
	epEvent.events = EPOLLIN;// | EPOLLOUT;

	// TODO: replace with loop through all interface:port pairs
	// for config.interface_port_pairs ...
		gaiErr = getaddrinfo(this->interface, this->port, &hints, &res);
		if (gaiErr != 0)
			throw std::runtime_error(gai_strerror(gaiErr));
		// for (struct addrinfo	*cur = data.addrinfoRes; cur != NULL; cur = cur->ai_next)
		// 	std::cout << "ai_canonname: " << cur->ai_canonname << std::endl;
		// arguments are equivalent to AF_INET, SOCK_STREAM, IPPROTO_TCP
		fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (fd == -1) {
			freeaddrinfo(res);
			throw std::runtime_error(std::strerror(errno));
		}
		this->listenSockets.push_back(fd);
		// TODO: use setsockopt here to avoid the error: Address already in use
		if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
			freeaddrinfo(res);
			throw std::runtime_error(std::strerror(errno));
		}
		freeaddrinfo(res);
		// TODO: backlog
		if (listen(fd, 10) == -1)
			throw std::runtime_error(std::strerror(errno));
		epEvent.data.fd = fd;
		if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &epEvent) == -1)
			throw std::runtime_error(std::strerror(errno));
}

void	Server::serverStartup()
{
	this->epollFd = epoll_create1(0);
	if (this->epollFd == -1)
		throw std::runtime_error(std::strerror(errno));
	this->initListenSockets();
	this->eventLoop();
}
