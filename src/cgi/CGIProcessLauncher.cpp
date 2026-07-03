# include "CGIProcessLauncher.hpp"
# include "Server.hpp"

CGIProcessLauncher::CGIProcessLauncher() : server(NULL) {}

CGIProcessLauncher::CGIProcessLauncher(Server	*server) :
	server(server)//, path(NULL), argv(NULL), envp(NULL)
{
	this->stdinPipe[0] = this->stdinPipe[1] = -1;
	this->stdoutPipe[0] = this->stdoutPipe[1] = -1;
}

CGIProcessLauncher::CGIProcessLauncher(const CGIProcessLauncher &other) { (void)other; }
CGIProcessLauncher	&CGIProcessLauncher::operator=(const CGIProcessLauncher &other)
{
	if (this == &other)
		return (*this);
	this->server = other.server;
	this->path = other.path;
	// copy argv and envp
	this->stdinPipe[0] = other.stdinPipe[0];
	this->stdinPipe[1] = other.stdinPipe[1];
	this->stdoutPipe[0] = other.stdoutPipe[0];
	this->stdoutPipe[1] = other.stdoutPipe[1];
	return (*this);
}

CGIProcessLauncher::~CGIProcessLauncher() {}

/*
void	CGIProcessLauncher::createArgs(char *argv[3], char **envp,
	std::map<std::string, std::string>	&envpMap, std::string file)
{
	// TODO: should the alloc's be protected?
	//
	// char	*path = new char [file.size() + 1];
	// std::strcpy(path, file.c_str());
	std::string	interpreter = "/usr/bin/python3";

	argv[0] = interpreter.c_str;
	argv[1] = file.c_str();
	argv[2] = NULL;
}
*/

char	**CGIProcessLauncher::getArray(std::vector<std::string> lst)
{
	char	**arr;

	if (lst.empty())
		return (NULL);
	arr = new char*[lst.size() + 1]();
	for (size_t	i = 0; i < lst.size(); ++i)
		arr[i] = (char *)lst[i].c_str();
	return (arr);
}

char	**CGIProcessLauncher::generateEnv(ClientConnection &client)
{
	std::ostringstream	content_len;
	std::vector<std::string>	env;

	/*
	meta-variable-name = "AUTH_TYPE" | "CONTENT_LENGTH" |
													"CONTENT_TYPE" | "GATEWAY_INTERFACE" |
													"PATH_INFO" | "PATH_TRANSLATED" |
													"QUERY_STRING" | "REMOTE_ADDR" |
													"REMOTE_HOST" | "REMOTE_IDENT" |
													"REMOTE_USER" | "REQUEST_METHOD" |
													"SCRIPT_NAME" | "SERVER_NAME" |
													"SERVER_PORT" | "SERVER_PROTOCOL" |
													"SERVER_SOFTWARE"
	*/
	content_len << client.request->getRequestBody().size() + 1;
	env.push_back("AUTH_TYPE=null")
	env.push_back("CONTENT_LENGTH=" + content_len.str());
	env.push_back("CONTENT_TYPE=" + client.request->getContentData().type);
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	return (this->getArray(env));
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

void	CGIProcessLauncher::newProcess(ClientConnection &client)
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
		t_CGIProcess	&process = this->server->cgiProcesses[this->stdoutPipe[0]];

		process.client = &client;
		process.pid = pid;
		// TODO: write to cgi process
		this->cleanUp();
		this->epEvent.events = EPOLLIN;
		this->epEvent.data.fd = this->stdoutPipe[0];
		if (epoll_ctl(this->server->getEpollFd(), EPOLL_CTL_ADD, this->stdoutPipe[0], &this->epEvent) == -1) {
			this->cleanUp(true);
			throw CGIError(std::strerror(errno));
		}
	}
	else {
		std::string	file, root;
		size_t	last_slash;
		std::vector<std::string>	args, env;
		char	**argv, **envp = NULL;


		// WARNING: this could be dangerous
		root = client.executor->availableLocation(client.request->getURI())->root.erase(0, 1);
		file = client.cgi_path;
		last_slash = file.find_last_of("/");
		if (last_slash != std::string::npos)
			file = file.substr(last_slash + 1);

		args.push_back("/usr/bin/python3");
		args.push_back(file);
		
		argv = this->getArray(args);
		envp = this->generateEnv(client);

		if (dup2(this->stdinPipe[0], STDIN_FILENO) == -1
				|| dup2(this->stdoutPipe[1], STDOUT_FILENO) == -1) {
			std::cerr << "CGI process failed: " << std::strerror(errno) << std::endl;
			this->cleanUp(true);
			std::exit(EXIT_FAILURE);
		}
		close(this->stdinPipe[1]);
		close(this->stdoutPipe[0]);

		if (chdir(root.c_str()) != -1)
			execve(file.c_str(), argv, envp);
		std::cerr << "CGI process failed: " << std::strerror(errno) << std::endl;

		delete[] argv;
		delete[] envp;
		this->cleanUp(true);
		std::exit(EXIT_FAILURE);
	}
}
