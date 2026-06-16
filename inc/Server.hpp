#ifndef SERVER_HPP
# define SERVER_HPP

# include "Config.hpp"
# include "../src/http/ClientConnection.hpp"
// CPP
# include <iostream>
# include <string>
# include <vector>
# include <map>
# include <stdexcept>
# include <cerrno>
# include <cstring> // for std::memset

// POSIX
# include <sys/socket.h>
# include <sys/epoll.h>
# include <stdlib.h>
# include <unistd.h>
#include <netdb.h>

class Server
{
	public:
		// TODO: repalce constructor arguments with config class
		// Server(const Config	config);
		Server(const char *interface, const char *port);
		~Server();

		void	serverStartup();
	private:
		Server();
		Server(const Server &other);
		Server	&operator=(const Server &other);

		// Config	config;
		// TODO: remove tmp
		const char	*interface;
		const char	*port;
		//
		std::vector<int>	listenSockets;
		std::map<int, ClientConnection>	clients;
		int	epollFd;

		void	initListenSockets();
		bool	isListenSock(int fd);
		void	eventLoop();
		void	handleNewClient(int listenFd);
		void	handleClientRead(int);
		void	handleClientWrite(int);
};

#endif // !SERVER_HPP
