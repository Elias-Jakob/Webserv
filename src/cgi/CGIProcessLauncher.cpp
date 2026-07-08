# include "CGIProcessLauncher.hpp"

// CGIProcessLauncher::CGIProcessLauncher() : epollFd(NULL),
// 	cgiProcesses(NULL), path(NULL) {}

CGIProcessLauncher::CGIProcessLauncher(int &epollFd, std::map<int, t_CGIProcess> &cgiProcesses) :
	epollFd(epollFd), cgiProcesses(cgiProcesses)
{
	std::memset(this->stdinPipe, -1, sizeof(this->stdinPipe));
	std::memset(this->stdoutPipe, -1, sizeof(this->stdoutPipe));
}

CGIProcessLauncher::~CGIProcessLauncher() {}

void	CGIProcessLauncher::cleanUp(bool closeAll = false)
{
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

void	CGIProcessLauncher::newProcess(ClientConnection &client)
{
	// TODO:
	// 1. is it viable to just use one pipe for in and out? if not why?
	if (pipe(this->stdinPipe) == -1)
		throw CGIError(std::strerror(errno));
	if (pipe(this->stdoutPipe) == -1
			|| fcntl(this->stdoutPipe[0], F_SETFL, O_NONBLOCK)) {
		this->cleanUp();
		throw CGIError(std::strerror(errno));
	}
	pid = fork();
	if (pid < 0) {
		this->cleanUp(true);
		throw CGIError(std::strerror(errno));
	}
	else if (pid) {
		// 1. add to cgi process info to processes list
		// 2. add cgi stdout to list of interest of epoll
		// 3. write request body into the cgi process
		// 3. (not in here but in parent process) read from the cgis out -> pass to cgi body parsing
		t_CGIProcess	&process = this->cgiProcesses[this->stdoutPipe[0]];

		process.client = &client;
		process.pid = pid;
		this->cleanUp();
		this->epEvent.events = EPOLLIN;
		this->epEvent.data.fd = this->stdoutPipe[0];
		if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->stdoutPipe[0], &this->epEvent) == -1) {
			this->cleanUp(true);
			throw CGIError(std::strerror(errno));
		}
	}
	else
		this->runChildProcess(client);
}
