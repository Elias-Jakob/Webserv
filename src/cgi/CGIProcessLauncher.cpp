# include "CGIProcessLauncher.hpp"

// CGIProcessLauncher::CGIProcessLauncher() : epollFd(NULL),
// 	cgiPipes(NULL), path(NULL) {}

CGIProcessLauncher::CGIProcessLauncher(int &epollFd, std::map<int, ClientConnection&> &cgiPipes) :
	epollFd(epollFd), cgiPipes(cgiPipes)
{
	std::memset(this->stdinPipe, -1, sizeof(this->stdinPipe));
	std::memset(this->stdoutPipe, -1, sizeof(this->stdoutPipe));
}

CGIProcessLauncher::~CGIProcessLauncher() {}

void	CGIProcessLauncher::cleanUp(bool closeAll)// = true)
{
	if (closeAll) {
		close(this->stdinPipe[1]);
		close(this->stdoutPipe[0]);
		this->stdoutPipe[0] = this->stdinPipe[1] = -1;
	}
	close(this->stdinPipe[0]);
	close(this->stdoutPipe[1]);
	this->stdinPipe[0] = this->stdoutPipe[1] = -1;
}

void	CGIProcessLauncher::newProcess(ClientConnection &client)
{
	// TODO:
	// 1. just a thought: is it viable to just use one pipe for in and out? if not why?
	// 2. is it necessary to set the in/out ends to non blocking?
	if (pipe(this->stdinPipe) == -1 || fcntl(this->stdinPipe[1], F_SETFD, FD_CLOEXEC) == -1 ||
			fcntl(this->stdinPipe[1], F_SETFL, O_NONBLOCK) == -1)
		throw CGIError(std::strerror(errno));
	if (pipe(this->stdoutPipe) == -1 || fcntl(this->stdoutPipe[0], F_SETFD, FD_CLOEXEC) == -1 ||
			fcntl(this->stdoutPipe[0], F_SETFL, O_NONBLOCK) == -1) {
		this->cleanUp();
		throw CGIError(std::strerror(errno));
	}
	pid = fork();
	if (pid < 0) {
		this->cleanUp();
	throw CGIError(std::strerror(errno));
	}
	else if (pid) {
		// 1. add to cgi process info to processes list
		// 2. add cgi stdout to list of interest of epoll
		// 3. write request body into the cgi process
		// 3. (not in here but in parent process) read from the cgis out -> pass to cgi body parsing

		// TODO: check if there's already a cgi running for the client
		client.cgiPid = pid;
		// Writing input to the cgi
		this->cgiPipes.insert(std::pair<int, ClientConnection&>(this->stdinPipe[1], client));
		client.cgiIn = this->stdinPipe[1];

		this->epEvent.events = EPOLLOUT;
		this->epEvent.data.fd = this->stdinPipe[1];
		if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->stdinPipe[1], &this->epEvent) == -1) {
			this->cleanUp();
			throw CGIError(std::strerror(errno));
		}

		// Reading output from the cgi
		this->cgiPipes.insert(std::pair<int, ClientConnection&>(this->stdoutPipe[0], client));
		client.cgiOut = this->stdoutPipe[0];

		this->epEvent.events = EPOLLIN;
		this->epEvent.data.fd = this->stdoutPipe[0];
		if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->stdoutPipe[0], &this->epEvent) == -1) {
			this->cleanUp();
			throw CGIError(std::strerror(errno));
		}
		this->cleanUp(false);
	}
	else
		this->runChildProcess(client);
}
