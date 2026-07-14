# include "Server.hpp"

Server::Server(t_Configs &configs) : configs(configs), cgiLauncher(epollFd, cgiPipes)
{}

Server::~Server()
{
	std::cout << "Destructing server..." << std::endl;
	for (std::vector<int>::iterator	it = this->listenSockets.begin();
			it != this->listenSockets.end(); ++it)
		if (*it != -1) close(*it);
	for (std::map<int, ClientConnection>::iterator	it = this->clients.begin();
			it != this->clients.end(); ++it)
		if (it->first != -1) close(it->first);
	if (this->epollFd != -1)
		close(this->epollFd);
}

void	Server::setupSocketAddr(struct addrinfo *res, int &fd)
{
	fd = -1;
	for (struct addrinfo	*cur = res; cur != NULL; cur = cur->ai_next) {
		fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (fd == -1) {
			if (errno == EAFNOSUPPORT || errno == EPROTONOSUPPORT) // || errno == EPROTOYTPE)
				continue;
			throw std::runtime_error(std::strerror(errno));
		}
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1 || fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
			throw std::runtime_error(std::strerror(errno));
		// TODO: setsockopt
		int opt = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			throw std::runtime_error(std::strerror(errno));
		//
		if (bind(fd, res->ai_addr, res->ai_addrlen) == 0)
			break ;
		close(fd);
		fd = -1;
	}
	if (fd == -1 || listen(fd, SOMAXCONN) == -1)
		throw std::runtime_error(std::strerror(errno));
	freeaddrinfo(res);
	res = NULL;
}

// TESTING: todo
bool	Server::initListenSockets()
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
	epEvent.events = EPOLLIN;

	// TODO: replace with loop through all interface:port pairs
	// for config.interface_port_pairs ...
	for (t_MultiStrMap::const_iterator	interface = this->configs.endpoints.begin();
			interface != this->configs.endpoints.end(); ++interface) {
		for (std::vector<std::string>::const_iterator	port = interface->second.begin();
				port != interface->second.end(); ++port) {
			res = NULL;
			gaiErr = getaddrinfo(interface->first.c_str(), (*port).c_str(), &hints, &res);
			try {
				if (gaiErr != 0)
					throw std::runtime_error(gai_strerror(gaiErr));
				this->setupSocketAddr(res, fd);
				epEvent.data.fd = fd;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &epEvent) == -1)
					throw std::runtime_error(std::strerror(errno));
				std::cout << "Listening endpoint " << interface->first << ":" << *port
					<< " has been sucessfully set up" << std::endl;
				this->listenSockets.push_back(fd);
			} catch (const std::runtime_error	&e) {
				std::cerr << "Failed to set up listening endpoint " << interface->first
					<< ":" << *port << ": " << e.what() << std::endl;
				if (res != NULL)
				freeaddrinfo(res);
				if (fd != -1)
					close(fd);
			}
		}
	}
	return (this->listenSockets.size() > 0);
}

void	Server::serverStartup()
{
	this->methodExecuter.setConfig(&this->configs);
	this->responseBuilder.setConfig(&this->configs);
	this->epollFd = epoll_create1(0);
	if (this->epollFd == -1 || fcntl(this->epollFd, F_SETFD, FD_CLOEXEC) == -1)
		throw std::runtime_error(std::strerror(errno));
	if (!this->initListenSockets())
		throw std::runtime_error("Failed to set up any listening sockets");
	this->eventLoop();
}
