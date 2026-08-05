# include "Epoll.hpp"

Epoll::Epoll() : fd(epoll_create1(EPOLL_CLOEXEC)) {
	if (this->fd == -1)
		throw std::runtime_error(std::strerror(errno));
}

Epoll::~Epoll() {
	if (this->fd != -1)
		close(this->fd);
}

void	Epoll::ctl(int fd, int op, uint32_t events) {
	struct epoll_event	epEvent;

	epEvent.events = events;
	epEvent.data.fd = fd;
	if (epoll_ctl(this->fd, op, epEvent.data.fd, &epEvent) == -1)
		throw std::runtime_error("epoll ctl failed: " + std::string(std::strerror(errno)));
};

