# include "Socket.hpp"

Socket::Socket(const struct addrinfo hints, const char *interface, const char *port) :
	fd(-1), res(NULL)
{
	int	addrResolved;

	addrResolved = getaddrinfo(interface, port, &hints, &this->res);
	if (addrResolved != 0)
		throw std::runtime_error(gai_strerror(addrResolved));
	// for (struct addrinfo	*cur = data.addrinfoRes; cur != NULL; cur = cur->ai_next)
	// 	std::cout << "ai_canonname: " << cur->ai_canonname << std::endl;
	// arguments are equivalent to AF_INET, SOCK_STREAM, IPPROTO_TCP
	this->fd = socket(this->res->ai_family, this->res->ai_socktype,
		this->res->ai_protocol);
	if (this->fd == -1)
		throw std::runtime_error(std::strerror(errno));
	// TODO: use setsockopt here to avoid the error: Address already in use
	if (bind(this->fd, this->res->ai_addr, this->res->ai_addrlen) == -1)
		throw std::runtime_error(std::strerror(errno));
	freeaddrinfo(res);
	res = NULL;
	// TODO: backlog
	if (listen(this->fd, 10) == -1)
		throw std::runtime_error(std::strerror(errno));
}

Socket::~Socket()
{
	if (fd != -1)
		close(fd);
	if (res)
		freeaddrinfo(res);
}

const int	Socket::getFd() const
{
	return (this->fd);
}
