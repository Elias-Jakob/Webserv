# include "Server.hpp"

Server::Server(std::vector<t_Configs> &configs) : configs(configs), epoll(), cgiLauncher(epoll, cgiPipes)
{}

Server::~Server()
{
	std::cout << "Destructing server..." << std::endl;
	for (std::vector<int>::iterator	it = this->listenSockets.begin();
			it != this->listenSockets.end(); ++it)
		if (*it != -1) close(*it);
}

int	Server::setupSocketAddr(const char *interface, const char *port)
{
	struct addrinfo	*res;
	int gaierrno, fd, opt;

	gaierrno = getaddrinfo(interface, port, &this->addrHints, &res);
	if (gaierrno != 0)
		throw std::runtime_error(gai_strerror(gaierrno));
	for (struct addrinfo	*cur = res; cur != NULL; cur = cur->ai_next) {
		fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
		if (fd == -1)
			continue;
		opt = 1;
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1 ||
				fcntl(fd, F_SETFD, FD_CLOEXEC) == -1 ||
				// INFO: level = Socket Option Level; optname = socket option reuse address; optval = on (1); optsize = sizeof(int); optvalcould be any other type meaning we need to know how big it is
				setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
			freeaddrinfo(res);
			close(fd);
			throw std::runtime_error(std::strerror(errno));
		}
		if (bind(fd, cur->ai_addr, cur->ai_addrlen) == 0)
			break ;
		close(fd);
		fd = -1;
	}
	freeaddrinfo(res);
	if (fd == -1)
		throw std::runtime_error(std::strerror(errno));
	if (listen(fd, SOMAXCONN) == -1) {
		close(fd);
		throw std::runtime_error(std::strerror(errno));
	}
	return (fd);
}

void	Server::initListenSocket(std::vector<t_Configs>::iterator &conf)
{
	int	fd;

	for (t_MultiStrMap::const_iterator	interface = conf->endpoints.begin();
			interface != conf->endpoints.end(); ++interface) {
		for (std::vector<std::string>::const_iterator	port = interface->second.begin();
				port != interface->second.end(); ++port) {
			fd = -1;
			try {
				fd = this->setupSocketAddr(interface->first.c_str(), port->c_str());
				this->epoll.ctl(fd, EPOLL_CTL_ADD, EPOLLIN);
			} catch (const std::runtime_error	&e) {
				if (fd != -1) close(fd);
				std::cerr << "Failed to set up listening endpoint " << interface->first
					<< ":" << *port << ": " << e.what() << std::endl;
				continue ;
			}
			this->listenSockets.push_back(fd);
			this->listenFdToConfigIndex[fd] = std::distance(this->configs.begin(), conf);
			this->listenFdToInterface[fd] = interface->first + ":" + *port;
			std::cout << "Listening endpoint " << interface->first << ":" << *port
				<< " has been sucessfully set up" << std::endl;
		}
	}
}

void	Server::serverStartup()
{
	this->methodExecuter.setConfig(this->configs);
	this->responseBuilder.setConfig(this->configs);
	// INFO: this->addrHints indicate what the returned socket address will be used for
	std::memset(&this->addrHints, 0, sizeof(addrinfo));
	this->addrHints.ai_family = AF_UNSPEC; // dont care if a IPv4 or IPv6
	this->addrHints.ai_socktype = SOCK_STREAM; // we want a stable connection base socket
	this->addrHints.ai_protocol = IPPROTO_TCP; // redundant: by chosing SOCK_STREAM as socktype the protocol would defualt to TCP
	this->addrHints.ai_flags = AI_PASSIVE; // indicates that the returned socket address structure is intended for use in a call to bind
	for (std::vector<t_Configs>::iterator	conf = this->configs.begin();
			conf != this->configs.end(); ++conf)
		this->initListenSocket(conf);
	if (this->listenSockets.size() < 1)
		throw std::runtime_error("Failed to set up any listening sockets");
	this->eventLoop();
}
