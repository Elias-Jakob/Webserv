# include "Socket.hpp"

Socket::Socket(const int epollFd, const struct addrinfo hints, const char *interface, const char *port) :
	fd(-1), listenSock(true)
{

	struct addrinfo	*res;
	int	addrResolved;

	addrResolved = getaddrinfo(interface, port, &hints, &res);
	if (addrResolved != 0)
		throw std::runtime_error(gai_strerror(addrResolved));
	// for (struct addrinfo	*cur = data.addrinfoRes; cur != NULL; cur = cur->ai_next)
	// 	std::cout << "ai_canonname: " << cur->ai_canonname << std::endl;
	// arguments are equivalent to AF_INET, SOCK_STREAM, IPPROTO_TCP
	this->fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (this->fd == -1) {
		freeaddrinfo(res);
		throw std::runtime_error(std::strerror(errno));
	}
	// TODO: use setsockopt here to avoid the error: Address already in use
	if (bind(this->fd, res->ai_addr, res->ai_addrlen) == -1) {
		freeaddrinfo(res);
		throw std::runtime_error(std::strerror(errno));
	}
	freeaddrinfo(res);
	// TODO: backlog
	if (listen(this->fd, 10) == -1)
		throw std::runtime_error(std::strerror(errno));
	this->addToInterestList(epollFd);
}

Socket::Socket(const int epollFd, const int fd) : fd(fd), listenSock(false)
{

	this->addToInterestList(epollFd);
}

Socket::~Socket()
{
	if (this->fd != -1) {
		// TODO: clean up the poll
		// epoll_ctl();
		close(this->fd);
	}
}

// Add the sockets fd to the interest list of epoll
void	Socket::addToInterestList(int epollFd)
{
	struct epoll_event	epEvent;

	epEvent.events = EPOLLIN | EPOLLOUT;
	epEvent.data.fd = this->fd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, this->fd, &event) == -1)
		throw std::runtime_error(std::strerror(errno));
}

const bool	Socket::isListenSock() const
{ return (this->listenSock); }

const int	Socket::getFd() const
{ return (this->fd); }
