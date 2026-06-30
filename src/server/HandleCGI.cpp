# include "Server.hpp"
# include <cstdlib> // std::exit

static void	closeFds(int fds[2])
{
	close(fds[0]);
	close(fds[1]);
}

void	Server::launchCGIProcess(ClientConnection &client)
{
	char	*path = new char [client.cgi_path.size() + 1];
	std::strcpy(path, client.cgi_path.c_str());
	char	*argv[] = { path, NULL };
	char	*envp[] = { NULL };
	pid_t	pid;
	int	inPipe[2], outPipe[2];
	struct epoll_event	epEvent;

	if (pipe(inPipe) == -1)
		throw std::runtime_error(std::strerror(errno));
	if (pipe(outPipe) == -1) {
		closeFds(inPipe);
		throw std::runtime_error(std::strerror(errno));
	}
	pid = fork();
	if (pid == -1) {
		closeFds(inPipe);
		closeFds(outPipe);
		throw std::runtime_error(std::strerror(errno));
	}
	else if (pid) {
		// TODO: write to cgi process

		this->cgis[outPipe[0]].pid = pid;
		this->cgis[outPipe[0]].client = client;
		epEvent.events = EPOLLIN;
		epEvent.data.fd = outPipe[0];
		if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, outPipe[0], &epEvent) == -1)
			throw std::runtime_error(std::strerror(errno));
		closeFds(inPipe);
		close(outPipe[1]);
	}
	else {
		if (dup2(inPipe[0], STDIN_FILENO) == -1 || dup2(outPipe[1], STDOUT_FILENO) == -1) {
			closeFds(inPipe);
			closeFds(outPipe);
			std::exit(EXIT_FAILURE);
		}
		close(inPipe[1]);
		close(outPipe[0]);
		execve(client.cgi_path.c_str(), argv, envp);
		closeFds(inPipe);
		closeFds(outPipe);
		std::exit(EXIT_FAILURE);
	}

	// - create envp
	// - in = pipe, out = pipe
	// - fork
	// - in parent process -> save CGIProcess instance (client, pid, ...?)
	// - dup2(child_stdin, in), dup2(child_stdout, out)
	// - this->cgis[out] = CGIProcess
	// - execve(client.cgi_path, envp)
	// throw if there's any issue
	
}
