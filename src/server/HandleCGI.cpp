# include "Server.hpp"


class CGI
{
	public:
		CGI(ClientConnection &client, t_CGIProcess &cgi);
		~CGI();
		void	launchCGIProcess();
	private;
		ClientConnection	&client;
		t_CGIProcess	&cgi;
		char	*path;
		char	**argv;
		char	**envp;
		int	stdinPipe[2];
		int	stdoutPipe[2];
		void	createArgs();
};


CGIProcess::CGIProcess(ClientConnection &client, t_CGIProcess &cig) :
	client(client), cgi(cgi), path(NULL), argv(NULL), envp(NULL)
{
	this->stdinPipe[0] = -1;
	this->stdinPipe[1] = -1;
	this->stdoutPipe[0] = -1;
	this->stdoutPipe[1] = -1;
}

CGIProcess::~CGIProcess()
{
	delete this->path;
	delete[] this->envp;
	delete[] this->argv;
	if (this->stdinPipe[0] != -1)
		close(this->stdinPipe[0]);
	if (this->stdinPipe[1] != -1)
		close(this->stdinPipe[1]);
	if (this->stdoutPipe[1] != -1)
		close(this->stdoutPipe[1]);
}

static void	closeFds(int fds[2])
{
	close(fds[0]);
	close(fds[1]);
}

static void	createArgs(char **argv, char **envp, ClientConnection &client)
{
	// TODO: should the alloc's be protected?
	char	*path = new char [client.cgi_path.size() + 1];

	std::strcpy(path, client.cgi_path.c_str());
	argv = new char*[2]();
	argv[0] = path;
	envp = new char*[1]();
}

void	Server::launchCGIProcess(ClientConnection &client)
{
	// TODO: Create propper argv and envp
	char	**argv;
	char	**envp;
	int	inPipe[2], outPipe[2];
	struct epoll_event	epEvent;

	createArgs(argv, envp, client);
	if (pipe(inPipe) == -1)
		throw CGIError(std::strerror(errno));
	if (pipe(outPipe) == -1) {
		closeFds(inPipe);
		throw CGIError(std::strerror(errno));
	}
	// i think i only need this in the parent
	t_CGIProcess	&cig = this->cigs[outPipe[0]](pid, client);
	pid = fork();
	if (pid == -1) {
		closeFds(inPipe);
		closeFds(outPipe);
		throw CGIError(std::strerror(errno));
	}
	else if (pid) {
		// TODO: write to cgi process
		close(outPipe[1]);
		closeFds(inPipe);
		epEvent.events = EPOLLIN;
		epEvent.data.fd = outPipe[0];
		if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, outPipe[0], &epEvent) == -1) {
			close(outPipe[0]);
			throw CGIError(std::strerror(errno));
		}
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
