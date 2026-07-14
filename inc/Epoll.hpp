#ifndef EPOLLWRAPPER_HPP
# define EPOLLWRAPPER_HPP
# include <fcntl.h>
# include <string>
# include <stdexcept>
# include <cerrno>
# include <cstring>

# include <sys/epoll.h>
# include <unistd.h>

class Epoll
{
	public:
		Epoll();
		~Epoll();
		
		void	ctl(int fd, int op, uint32_t events);
		const int	fd;
	private:
		Epoll(const Epoll &other);
		Epoll	&operator=(const Epoll &other);
};

#endif // !EPOLLWRAPPER_HPP
