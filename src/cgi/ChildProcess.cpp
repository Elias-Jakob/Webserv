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
	this->envs.push_back("PATH_INFO=" + request->getPathInfo());
	if (!request->getPathInfo().empty())
		this->envs.push_back("PATH_TRANSLATED=" + request->getPathTranslated());
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
	this->envs.push_back("REMOTE_ADDR=" + client.remoteAddr);
	for (t_MultiStrMap::iterator	it = headers.begin(); it != headers.end(); ++it)
		this->envs.push_back(cgi::toMetaFormat(it->first) + "=" + cgi::strJoin(it->second));

	// session-management: expose the session id and its current key/value bag.
	// The server has no notion of what these keys mean (login-state, cart, ...);
	// it only provides the storage/transport mechanism.
	if (client.sessionManager && !client.sessionCookie.empty()) {
		this->envs.push_back("SESSION_ID=" + client.sessionCookie);
		std::map<std::string, std::string> sessionData =
			client.sessionManager->getAllData(client.sessionCookie);
		for (std::map<std::string, std::string>::iterator it = sessionData.begin();
				it != sessionData.end(); ++it) {
			std::string key = it->first;
			for (std::string::iterator c = key.begin(); c != key.end(); ++c)
				*c = std::toupper(static_cast<unsigned char>(*c));
			this->envs.push_back("SESSION_" + key + "=" + it->second);
		}
	}
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
