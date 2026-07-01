#ifndef SERVER_HPP
# define SERVER_HPP

# include "ConfigFileParser.hpp"
# include "ClientConnection.hpp"
# include "MethodExecuter.hpp"
# include "CGIProcessLauncher.hpp"
# include "CGIError.hpp"

// CPP
# include <iostream>
# include <string>
# include <vector>
# include <map>
# include <stdexcept>
# include <cerrno>
# include <cstring>
# include <cstdlib> // std::exit
# include <algorithm> // for std::find

// POSIX
# include <sys/socket.h>
# include <sys/epoll.h>
# include <stdlib.h>
# include <unistd.h>
# include <netdb.h>
# include <signal.h>
# include <fcntl.h>

extern sig_atomic_t	sigFlag;

class Server
{
	public:
		Server(t_Configs &configs);
		~Server();

		void	serverStartup();
	private:
		Server();
		Server(const Server &other);
		Server	&operator=(const Server &other);

		t_Configs	&configs;
		MethodExecuter	methodExecuter;
		ResponseBuilder	responseBuilder;
		CGIProcessLauncher	cgiLauncher;
		std::vector<int>	listenSockets;
		std::map<int, ClientConnection>	clients;
		std::map<int, t_CGIProcess>	cgiProcesses;
		int	epollFd;

		void	setupSocketAddr(struct addrinfo *res, int &fd);
		bool	initListenSockets();
		void	eventLoop();
		void	handleNewClient(int listenFd, MethodExecuter &methodExecuter, ResponseBuilder &responseBuilder);
		void	handleClientRead(int);
		void	handleClientWrite(int);
		void	removeClient(ClientConnection &client);
		void	timeoutInactiveClients();
		void	readFromCGI(int fd);
		void	killCGIProcesses(ClientConnection &client);
};

// DEBUG HELPERS
//

#endif // !SERVER_HPP
