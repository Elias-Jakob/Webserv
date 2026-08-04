#ifndef CGIPROCESSLAUNCHER_HPP
# define CGIPROCESSLAUNCHER_HPP

# include "Epoll.hpp"
# include "ClientConnection.hpp"
# include "CGIError.hpp"
# include "cgi.hpp"
# include "utils.hpp"

class CGIProcessLauncher
{
	public:
		CGIProcessLauncher(Epoll &epoll, std::map<int, ClientConnection*> &cgiPipes);
		~CGIProcessLauncher();

		void	newProcess(ClientConnection &client);
	private:
		CGIProcessLauncher();
		CGIProcessLauncher(const CGIProcessLauncher &other);
		CGIProcessLauncher	&operator=(const CGIProcessLauncher &other);

		Epoll	&epoll;
		std::map<int, ClientConnection*>	&cgiPipes;
		// struct epoll_event	epEvent;
		pid_t	pid;
		int	stdinPipe[2];
		int	stdoutPipe[2];
		std::vector<std::string>	args;
		std::vector<std::string>	envs;

		// void	cleanUp(bool closeAll);
		void	cleanUp(bool closeAll = true);
		void	runChildProcess(const ClientConnection &client);

		char	**createArgv(const ClientConnection &client);
		char	**createEnvp(const ClientConnection &client);
};

#endif
