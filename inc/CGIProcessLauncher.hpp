#ifndef CGIPROCESSLAUNCHER_HPP
# define CGIPROCESSLAUNCHER_HPP

# include <sys/epoll.h>
# include "ClientConnection.hpp"
# include "CGIError.hpp"

typedef struct	s_CGIProcess
{
	ClientConnection	*client;
	int	pid;
} t_CGIProcess;

class CGIProcessLauncher
{
	public:
		CGIProcessLauncher();
		CGIProcessLauncher(int epollFd);
		CGIProcessLauncher(const CGIProcessLauncher &other);
		CGIProcessLauncher	&operator=(const CGIProcessLauncher &other);
		~CGIProcessLauncher();
		void	newProcess(ClientConnection &client, std::map<int, t_CGIProcess> &cgiProcesses);
	private:
		struct epoll_event	epEvent;
		int	epollFd;
		std::string	*path;
		pid_t	pid;
		// char	**argv;
		// char	**envp;
		int	stdinPipe[2];
		int	stdoutPipe[2];

		void	createArgs(char **argv, char **envp, ClientConnection &client);
		void	cleanUp(bool closeAll);
};

#endif
