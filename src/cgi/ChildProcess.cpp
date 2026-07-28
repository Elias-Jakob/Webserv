# include "CGIProcessLauncher.hpp"

char	**CGIProcessLauncher::createArgv(const ClientConnection &client)
{
	std::string	file;
	size_t	last_slash;

	file = client.cgi_path;
	last_slash = file.find_last_of("/");
	if (last_slash != std::string::npos)
		file = file.substr(last_slash + 1);
	if (client.request->getFileExtension() == ".py")
		this->args.push_back("/usr/bin/python3");
	else if (client.request->getFileExtension() == ".php")
		this->args.push_back("/usr/bin/php-cgi");
	else
		this->args.push_back("./" + file);
	if (client.request->getFileExtension() == ".py" || client.request->getFileExtension() == ".php")
		this->args.push_back(file);
	return (cgi::getArray(args));
}

char	**CGIProcessLauncher::createEnvp(const ClientConnection &client)
{
	HttpRequest	*request = client.request;
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
	// Done count = 12; total = 17
	this->envs.push_back("GATEWAY_INTERFACE=CGI/1.1");
	this->envs.push_back("SERVER_PROTOCOL=HTTP/1.1");
	this->envs.push_back("SERVER_SOFTWARE=webserv/1.0");
	if (!request->getRequestBody().empty())
		this->envs.push_back("CONTENT_LENGTH=" +
			utils::numToStr(request->getRequestBody().size() + 1));
	if (!request->getContentData().type.empty())
		this->envs.push_back("CONTENT_TYPE=" + request->getContentData().type);
	this->envs.push_back("REQUEST_METHOD=" + request->getMethod());
	this->envs.push_back("SCRIPT_NAME=" + request->getScriptName());

	this->envs.push_back("QUERY_STRING=" + request->getRequestLine().queryStr);
	// TODO: Clarify if PATH_TRANSLATED and AUTH_TYPE are required
	// PATH_INFO = the path that may follow after the cgi script; PATH_TRANSLATED = PATH_INFO mapped to the real filesystem path
	this->envs.push_back("PATH_INFO=" + request->getPathInfo());
	// if (!request->getPathInfo().empty())
	// 	this->envs.push_back("PATH_TRANSLATED=" + );
	headers = request->getRequestHeaders();
	if (!headers["host"].empty()) {
		std::string	host = headers.at("host").front();
		this->envs.push_back("SERVER_NAME=" + host);
		size_t	portPos = host.find_first_of(":");
		if (portPos != std::string::npos)
			this->envs.push_back("SERVER_PORT=" + host.substr(portPos + 1));
	}
	if (request->getFileExtension() == ".php") {
		this->envs.push_back("REDIRECT_STATUS=200");
		this->envs.push_back("SCRIPT_FILENAME=" + this->args.back());
	}
	// These are still left on the todo side:
	// "AUTH_TYPE"    |               | "PATH_TRANSLATED" |
	//                |               | "REMOTE_HOST"     |
	// "REMOTE_IDENT" | "REMOTE_USER" |
	// TODO: REMOTE_ADDR in order to provide this info i would need to store the client's addr_info after accept
	this->envs.push_back("REMOTE_ADDR=" + client.remoteAddr);
	for (t_MultiStrMap::iterator	it = headers.begin(); it != headers.end(); ++it)
		this->envs.push_back(cgi::toMetaFormat(it->first) + "=" + cgi::strJoin(it->second));
	return (cgi::getArray(this->envs));
}

void	CGIProcessLauncher::runChildProcess(const ClientConnection &client)
{
	char	**argv = NULL, **envp = NULL;
	std::string	cgiDir;

	if (client.cgi_path.find("/") != std::string::npos)
		cgiDir = client.cgi_path.substr(0, client.cgi_path.find_last_of("/"));
	if (dup2(this->stdinPipe[0], STDIN_FILENO) == -1
			|| dup2(this->stdoutPipe[1], STDOUT_FILENO) == -1) {
		this->cleanUp();
		throw std::runtime_error("dup2: " + std::string(std::strerror(errno)));
	}
	close(this->stdinPipe[0]);
	close(this->stdoutPipe[1]);
	try {
		if (!cgiDir.empty() && chdir(cgiDir.c_str()) == -1)
			throw std::runtime_error("chdir: " + std::string(std::strerror(errno)));
		argv = this->createArgv(client);
		envp = this->createEnvp(client);
		execve(argv[0], argv, envp);
		throw std::runtime_error("execve: " + std::string(std::strerror(errno)));
	}
	catch (const std::exception &e) {
		delete[] argv;
		delete[] envp;
		this->cleanUp();
		std::cerr << "CGI process failed: " << e.what()
			<< " (" << client.remoteAddr << ")" << std::endl;
		throw CGIError();
	}
}
