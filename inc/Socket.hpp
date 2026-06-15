#ifndef SOCKET_HPP
# define SOCKET_HPP


class Socket
{
	public:
		// maybe just pass the config class here instead
		Socket(const struct addrinfo &hints, const char *interface, const char *port);
		~Socket();
		
		const int	getFd() const;
	private:
		Socket();
		Socket(const Socket &other);
		Socket	&operator=(const Socket &other);

		int	fd;
		struct addrinfo	*res;
};

#endif // !SOCKET_HPP
