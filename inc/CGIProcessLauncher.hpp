#ifndef CGIPROCESSLAUNCHER_HPP
# define CGIPROCESSLAUNCHER_HPP

# include <sys/epoll.h>
# include "ClientConnection.hpp"
# include "CGIError.hpp"

class Server;

typedef struct	s_CGIProcess
{
	ClientConnection	*client;
	int	pid;
} t_CGIProcess;

class CGIProcessLauncher
{
	public:
		CGIProcessLauncher();
		CGIProcessLauncher(Server *server);
		CGIProcessLauncher(const CGIProcessLauncher &other);
		CGIProcessLauncher	&operator=(const CGIProcessLauncher &other);
		~CGIProcessLauncher();
		void	newProcess(ClientConnection &client);
	private:
		struct epoll_event	epEvent;
		Server	*server;
		std::string	*path;
		pid_t	pid;
		// char	**argv;
		// char	**envp;
		int	stdinPipe[2];
		int	stdoutPipe[2];

		void	createArgs(char *const argv[3], char **envp,
			std::map<std::string, std::string>	&envpMap, std::string file);
		void	cleanUp(bool closeAll);
};

#endif
