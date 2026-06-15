#ifndef SOCKET_HPP
# define SOCKET_HPP

class Socket
{
	public:
		// maybe just pass the config class here instead
		Socket(const int epollFd, const struct addrinfo &hints, const char *interface, const char *port);
		~Socket();
		
		const int	getFd() const;
		const bool	isListenSock() const;
	private:
		Socket();
		Socket(const Socket &other);
		Socket	&operator=(const Socket &other);

		bool	listenSock; // Indicates whether it's a listening socket or not
		int	fd;
};

#endif // !SOCKET_HPP
