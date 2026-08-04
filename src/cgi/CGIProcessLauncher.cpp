# include "CGIProcessLauncher.hpp"

CGIProcessLauncher::CGIProcessLauncher(Epoll &epoll, std::map<int, int> &cgiPipes) :
	epoll(epoll), cgiPipes(cgiPipes)
{
	std::memset(this->stdinPipe, -1, sizeof(this->stdinPipe));
	std::memset(this->stdoutPipe, -1, sizeof(this->stdoutPipe));
}

CGIProcessLauncher::~CGIProcessLauncher()
{
	this->cleanUp(true);
}

void	CGIProcessLauncher::cleanUp(bool closeAll)
{
	if (closeAll) {
		if (this->stdinPipe[1] != -1) close(this->stdinPipe[1]);
		if (this->stdoutPipe[0] != -1) close(this->stdoutPipe[0]);
		this->stdoutPipe[0] = this->stdinPipe[1] = -1;
	}
	if (this->stdinPipe[0] != -1) close(this->stdinPipe[0]);
	if (this->stdoutPipe[1] != -1) close(this->stdoutPipe[1]);
	this->stdinPipe[0] = this->stdoutPipe[1] = -1;
}

void	CGIProcessLauncher::newProcess(ClientConnection &client)
{
	client.cgiStartTime = std::time(NULL);
	if (pipe(this->stdinPipe) == -1 || pipe(this->stdoutPipe) == -1 ||
			fcntl(this->stdinPipe[1], F_SETFD, FD_CLOEXEC) == -1 ||
			fcntl(this->stdinPipe[1], F_SETFL, O_NONBLOCK) == -1 ||
			fcntl(this->stdoutPipe[0], F_SETFD, FD_CLOEXEC) == -1 ||
			fcntl(this->stdoutPipe[0], F_SETFL, O_NONBLOCK) == -1) {
		this->cleanUp();
		throw std::runtime_error(std::strerror(errno));
	}
	pid = fork();
	if (pid < 0) {
		this->cleanUp();
		throw std::runtime_error(std::strerror(errno));
	}
	else if (pid) {
		std::cout << "Child process id = " << pid << std::endl;
		client.cgiPid = pid;
		client.cgiIn = this->stdinPipe[1];
		client.cgiOut = this->stdoutPipe[0];
		this->cleanUp(false);
		client.cgiWrittenBytes = 0;
		this->cgiPipes.insert(std::pair<int, int>(client.cgiIn, client.fd));
		this->cgiPipes.insert(std::pair<int, int>(client.cgiOut, client.fd));
		this->epoll.ctl(client.cgiIn, EPOLL_CTL_ADD, EPOLLOUT);
		this->epoll.ctl(client.cgiOut, EPOLL_CTL_ADD, EPOLLIN);
	}
	else
		this->runChildProcess(client);
}
