#ifndef SERVER_HPP
# define SERVER_HPP

# include "Config.hpp"
# include "Socket.hpp"
# include <iostream>
# include <vector>

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
		std::vector<Socket>	listSocks; // listening sockets
};

#endif // !SERVER_HPP
