# include "CGIProcessLauncher.hpp"

CGIProcessLauncher::CGIProcessLauncher() {}

CGIProcessLauncher::CGIProcessLauncher(int epollFd) :
	epollFd(epollFd)//, path(NULL), argv(NULL), envp(NULL)
{
	this->stdinPipe[0] = this->stdinPipe[1] = -1;
	this->stdoutPipe[0] = this->stdoutPipe[1] = -1;
}

CGIProcessLauncher::CGIProcessLauncher(const CGIProcessLauncher &other) { (void)other; }
CGIProcessLauncher	&CGIProcessLauncher::operator=(const CGIProcessLauncher &other)
{
	if (this == &other)
		return (*this);
	this->epollFd = other.epollFd;
	this->path = other.path;
	// copy argv and envp
	this->stdinPipe[0] = other.stdinPipe[0];
	this->stdinPipe[1] = other.stdinPipe[1];
	this->stdoutPipe[0] = other.stdoutPipe[0];
	this->stdoutPipe[1] = other.stdoutPipe[1];
	return (*this);
}

CGIProcessLauncher::~CGIProcessLauncher() {}

void	CGIProcessLauncher::createArgs(char **argv, char **envp, ClientConnection &client)
{
	// TODO: should the alloc's be protected?
	char	*path = new char [client.cgi_path.size() + 1];

	std::strcpy(path, client.cgi_path.c_str());
	argv = new char*[2]();
	argv[0] = path;
	envp = new char*[1]();
}

void	CGIProcessLauncher::cleanUp(bool closeAll = false)
{
	// delete this->path;
	// this->path = NULL;
	// delete[] this->argv;
	// this->argv = NULL;
	// delete[] this->envp;
	// this->envp = NULL;
	close(this->stdinPipe[0]);
	close(this->stdinPipe[1]);
	this->stdinPipe[0] = this->stdinPipe[1] = -1;
	if (closeAll) {
		close(this->stdoutPipe[0]);
		this->stdoutPipe[0] = -1;
	}
	close(this->stdoutPipe[1]);
	this->stdoutPipe[1] = -1;
}

void	CGIProcessLauncher::newProcess(ClientConnection &client, std::map<int, t_CGIProcess> &cgiProcesses)
{
	if (pipe(this->stdinPipe) == -1)
		throw CGIError(std::strerror(errno));
	if (pipe(this->stdoutPipe) == -1) {
		this->cleanUp();
		throw CGIError(std::strerror(errno));
	}
	pid = fork();
	if (pid == -1) {
		this->cleanUp(true);
		throw CGIError(std::strerror(errno));
	}
	else if (pid) {
		t_CGIProcess	&process = cgiProcesses[this->stdoutPipe[0]];

		process.client = &client;
		process.pid = pid;
		// TODO: write to cgi process
		this->cleanUp();
		this->epEvent.events = EPOLLIN;
		this->epEvent.data.fd = this->stdoutPipe[0];
		if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->stdoutPipe[0], &this->epEvent) == -1) {
			this->cleanUp(true);
			throw CGIError(std::strerror(errno));
		}
	}
	else {
		// just to make it work for now
		char	**argv = NULL;
		char	**envp = NULL;

		createArgs(argv, envp, client);
		if (dup2(this->stdinPipe[0], STDIN_FILENO) == -1
				|| dup2(this->stdoutPipe[1], STDOUT_FILENO) == -1) {
			this->cleanUp(true);
			std::exit(EXIT_FAILURE);
		}
		close(this->stdinPipe[1]);
		close(this->stdoutPipe[0]);
		execve(client.cgi_path.c_str(), argv, envp);
		//
		for (char *i = *argv; i != NULL; ++i)
			delete[] i;
		delete[] argv;
		for (char *i = *envp; i != NULL; ++i)
			delete[] i;
		delete[] envp;
		//
		this->cleanUp(true);
		std::exit(EXIT_FAILURE);
	}
}
