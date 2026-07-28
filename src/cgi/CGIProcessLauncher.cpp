# include "CGIProcessLauncher.hpp"

// CGIProcessLauncher::CGIProcessLauncher() : epollFd(NULL),
// 	cgiPipes(NULL), path(NULL) {}

CGIProcessLauncher::CGIProcessLauncher(Epoll &epoll, std::map<int, ClientConnection&> &cgiPipes) :
	epoll(epoll), cgiPipes(cgiPipes)
{
	std::memset(this->stdinPipe, -1, sizeof(this->stdinPipe));
	std::memset(this->stdoutPipe, -1, sizeof(this->stdoutPipe));
}

CGIProcessLauncher::~CGIProcessLauncher()
{
	this->cleanUp(true);
}

void	CGIProcessLauncher::cleanUp(bool closeAll)// = true)
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
	if (pipe(this->stdinPipe) == -1 || fcntl(this->stdinPipe[1], F_SETFD, FD_CLOEXEC) == -1 ||
			fcntl(this->stdinPipe[1], F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error(std::strerror(errno));
	if (pipe(this->stdoutPipe) == -1 || fcntl(this->stdoutPipe[0], F_SETFD, FD_CLOEXEC) == -1 ||
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
		// 1. add to cgi proce 1. add to cgi process info to processes list
		// 2. add cgi stdout to list of interest of epoll
		// 3. write request body into the cgi process
		// 3. (not in here but in parent process) read from the cgis out -> pass to cgi body parsing

		// TODO: check if there's already a cgi running for the client
		client.cgiPid = pid;
		// Writing input to the cgi
		this->cgiPipes.insert(std::pair<int, ClientConnection&>(this->stdinPipe[1], client));
		client.cgiIn = this->stdinPipe[1];

		this->epoll.ctl(this->stdinPipe[1], EPOLL_CTL_ADD, EPOLLOUT);

		// Reading output from the cgi
		this->cgiPipes.insert(std::pair<int, ClientConnection&>(this->stdoutPipe[0], client));
		client.cgiOut = this->stdoutPipe[0];

		this->epoll.ctl(this->stdoutPipe[0], EPOLL_CTL_ADD, EPOLLIN);
		this->cleanUp(false);
	}
	else
		this->runChildProcess(client);
}
