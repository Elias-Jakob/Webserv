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
# include <ctime>

// POSIX
# include <sys/socket.h>
# include <sys/epoll.h>
# include <sys/wait.h>
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
		std::vector<int>	listenSockets;
		std::map<int, ClientConnection>	clients;
		std::map<int, ClientConnection&>	cgiPipes;
		int	epollFd;
		CGIProcessLauncher	cgiLauncher;

		void	setupSocketAddr(struct addrinfo *res, int &fd);
		bool	initListenSockets();
		void	eventLoop();
		void	handleNewClient(int listenFd);
		void	handleIncoming(int);
		void	handleOutgoing(int);
		void	handleCGIOutput(int fd);
		void	writeRequestBodyToCGI(int fd);
		void	removeClient(ClientConnection &client);
		void	timeoutInactiveClients();
};

#endif // !SERVER_HPP
