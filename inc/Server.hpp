#ifndef SERVER_HPP
# define SERVER_HPP

# include "ConfigFileParser.hpp"
# include "Epoll.hpp"
# include "utils.hpp"
# include "ClientConnection.hpp"
# include "MethodExecuter.hpp"
# include "CGIProcessLauncher.hpp"
# include "CGIError.hpp"

// CPP
# include <iostream>
# include <string>
# include <vector>
# include <map>
# include <set>
# include <stdexcept>
# include <cerrno>
# include <cstring> // std::strerror
// # include <cstdlib> // std::exit
# include <algorithm> // for std::find
# include <ctime>

// POSIX
# include <sys/socket.h>
// # include <sys/epoll.h>
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
		Server(std::vector<t_Configs> &configs);
		~Server();

		void	serverStartup();
	private:
		Server();
		Server(const Server &other);
		Server	&operator=(const Server &other);

		std::vector<t_Configs>	&configs;
		Epoll	epoll;
		CGIProcessLauncher	cgiLauncher;
		MethodExecuter	methodExecuter;
		ResponseBuilder	responseBuilder;
		struct addrinfo	addrHints;
		std::vector<int>	listenSockets;
		std::map<int, ClientConnection>	clients;
		std::map<int, int>	cgiPipes;
		std::map<int, size_t>	listenFdToConfigIndex; // Maps listening fd → server config index
		std::map<int, std::string>	listenFdToInterface; // Maps listening fd → "127.0.0.1:8080"
		std::set<int>	justRemovedFds;

		int		setupSocketAddr(const char *interface, const char *port);
		void	initListenSocket(std::vector<t_Configs>::iterator &conf);
		void	eventLoop();
		void	acceptNewClient(int listenFd);
		void	handleIncoming(ClientConnection &caller);
		void	handleOutgoing(ClientConnection &caller);
		void	handleCGIOutput(ClientConnection &caller);
		void	writeRequestBodyToCGI(ClientConnection &caller);
		void	removeClient(ClientConnection &client);
		void	checkOnClients();
		void	cgiTimeoutResponse(ClientConnection &client);
		void	checkProcessStatus(ClientConnection &client);
		void	callEventHandler(const struct epoll_event &event);
		ClientConnection	*identifyEventCaller(int fd);
};

#endif // !SERVER_HPP
