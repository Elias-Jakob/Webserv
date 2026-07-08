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
		CGIProcessLauncher(int &epollFd, std::map<int, t_CGIProcess> &cgiProcesses);
		~CGIProcessLauncher();

		void	newProcess(ClientConnection &client);
	private:
		CGIProcessLauncher();
		CGIProcessLauncher(const CGIProcessLauncher &other);
		CGIProcessLauncher	&operator=(const CGIProcessLauncher &other);

		int	&epollFd;
		std::map<int, t_CGIProcess>	&cgiProcesses;
		struct epoll_event	epEvent;
		pid_t	pid;
		int	stdinPipe[2];
		int	stdoutPipe[2];
		std::vector<std::string>	args;
		std::vector<std::string>	envs;

		void	cleanUp(bool closeAll);
		void	runChildProcess(ClientConnection &client);

		char	**getArray(std::vector<std::string> &lst);
		char	**createArgv(ClientConnection &client);
		char	**createEnvp(HttpRequest*	request);


		// utils
		std::string	toMetaFormat(std::string	originalKey);
		std::string	vecJoin(std::vector<std::string> &vec);
};

#endif
