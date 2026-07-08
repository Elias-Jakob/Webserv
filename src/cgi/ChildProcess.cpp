# include "CGIProcessLauncher.hpp"

char	**CGIProcessLauncher::getArray(std::vector<std::string> &lst)
{
	char	**arr = NULL;

	if (lst.empty())
		return (NULL);
	arr = new char*[lst.size() + 1]();
	for (size_t	i = 0; i < lst.size(); ++i)
		arr[i] = (char *)lst[i].c_str();
	return (arr);
}

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
	return (this->getArray(args));
}

std::string	CGIProcessLauncher::vecJoin(std::vector<std::string> &vec)
{
	std::string	concat;

	for (std::vector<std::string>::const_iterator	it = vec.begin();
			it != vec.end(); ++it) {
		if (it != vec.begin())
			concat += ",";
		concat += *it;
	}
	return (concat);
}

std::string	CGIProcessLauncher::toMetaFormat(std::string	originalKey)
{
	for (std::string::iterator	it = originalKey.begin(); it != originalKey.end(); ++it) {
		*it = std::toupper(static_cast<unsigned char>(*it));
		if (*it == '-')
			*it = '_';
	}
	return ("HTTP_" + originalKey);
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

	for (t_MultiStrMap::iterator	it = headers.begin(); it != headers.end(); ++it)
		this->envs.push_back(this->toMetaFormat(it->first) + "=" + this->vecJoin(it->second));
	return (this->getArray(this->envs));
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
	// check with chris if this is safe or is any of those function could break
	cgiDir = client.executor->availableLocation(client.request->getURI())->root.erase(0, 1);
	//
	
	if (dup2(this->stdinPipe[0], STDIN_FILENO) == -1
			|| dup2(this->stdoutPipe[1], STDOUT_FILENO) == -1) {
		std::cerr << "CGI process failed: " << std::strerror(errno) << std::endl;
		this->cleanUp(true);
		std::exit(EXIT_FAILURE);
	}
	close(this->stdinPipe[1]);
	close(this->stdoutPipe[0]);
	if (chdir(cgiDir.c_str()) != -1)
		execve(argv[1], argv, envp);
	std::cerr << "CGI process failed: " << std::strerror(errno) << std::endl;
	delete[] argv;
	delete[] envp;
	this->cleanUp(true);
	std::exit(EXIT_FAILURE);
}
