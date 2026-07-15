# include "CGIProcessLauncher.hpp"

char	**CGIProcessLauncher::createArgv(ClientConnection &client)
{
	std::string	file;
	size_t	last_slash;

	// TODO: figure out a way to determine which interpreter is needed php/python (see file extension, we already got that bit of information just not here)
	// for now we will just assume that every cgi has a .py extension
	
	file = client.cgi_path;
	last_slash = file.find_last_of("/");
	if (last_slash != std::string::npos)
		file = file.substr(last_slash + 1);
	this->args.push_back("/usr/bin/python3");
	this->args.push_back(file);
	return (cgi::getArray(args));
}

char	**CGIProcessLauncher::createEnvp(HttpRequest* request)
{
	std::ostringstream	content_len;
	t_MultiStrMap	headers;
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
	// Done count = 9; total = 17
	this->envs.push_back("GATEWAY_INTERFACE=CGI/1.1");
	this->envs.push_back("SERVER_PROTOCOL=HTTP/1.1");
	this->envs.push_back("SERVER_SOFTWARE=webserv/1.0");
	if (!request->getRequestBody().empty()) {
		content_len << request->getRequestBody().size() + 1;
		this->envs.push_back("CONTENT_LENGTH=" + content_len.str());
	}
	if (!request->getContentData().type.empty())
		this->envs.push_back("CONTENT_TYPE=" + request->getContentData().type);
	this->envs.push_back("REQUEST_METHOD=" + request->getMethod());
	this->envs.push_back("SCRIPT_NAME=" + request->getURI());

	// TODO: QUERY_STRING
	// this->envs.push_back("QUERY_STRING=" + request->getQuery());
	// TODO: PATH_INFO & PATH_TRANSLATED
	// PATH_INFO = the path that may follow after the cgi script; PATH_TRANSLATED = PATH_INFO mapped to the real filesystem path
	this->envs.push_back("PATH_INFO=");
	this->envs.push_back("PATH_TRANSLATED=");
	//
	headers = request->getRequestHeaders();
	if (!headers["host"].empty()) {
		std::string	host = headers.at("host").front();
		this->envs.push_back("SERVER_NAME=" + host);
		size_t	portPos = host.find_first_of(":");
		if (portPos != std::string::npos)
			this->envs.push_back("SERVER_PORT=" + host.substr(portPos + 1));
	}
	// These are still left on the todo side:
	// "AUTH_TYPE"    | "PATH_INFO"   | "PATH_TRANSLATED" |
	// "QUERY_STRING" | "REMOTE_ADDR" | "REMOTE_HOST"     |
	// "REMOTE_IDENT" | "REMOTE_USER" |
	// TODO: REMOTE_ADDR in order to provide this info i would need to store the client's addr_info after accept
	for (t_MultiStrMap::iterator	it = headers.begin(); it != headers.end(); ++it)
		this->envs.push_back(cgi::toMetaFormat(it->first) + "=" + cgi::strJoin(it->second));
	return (cgi::getArray(this->envs));
}

void	CGIProcessLauncher::runChildProcess(ClientConnection &client)
{
	// 1. create argv (with path to interpreter)
	// 2. create envp
	// 3. dup2, chdir & execve
	char	**argv, **envp;
	std::string	cgiDir;

	argv = this->createArgv(client);
	envp = this->createEnvp(client.request);

	// WARNING: this could be dangerous
	// check with chris if this is safe or if any of those function could break
	// cgiDir = client.executor->availableLocation(client.request->getURI())->root.erase(0, 1);
	cgiDir = client.executor->availableLocation(client.request->getURI(), client._listeningInterface)->root.erase(0, 1);
	//
	
	if (dup2(this->stdinPipe[0], STDIN_FILENO) == -1
			|| dup2(this->stdoutPipe[1], STDOUT_FILENO) == -1) {
		this->cleanUp();
		throw std::runtime_error("CGI process failed: " + std::string(std::strerror(errno)));
	}
	close(this->stdinPipe[0]);
	close(this->stdoutPipe[1]);
	if (chdir(cgiDir.c_str()) != -1)
		execve(argv[0], argv, envp);
	delete[] argv;
	delete[] envp;
	this->cleanUp();
	throw std::runtime_error("CGI process failed: " + std::string(std::strerror(errno)));
}
