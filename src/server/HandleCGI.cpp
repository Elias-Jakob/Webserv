# include "Server.hpp"

void	Server::launchCGIProcess(ClientConnection &client)
{
	// - create envp
	// - in = pipe, out = pipe
	// - fork
	// - in parent process -> save CGIProcess instance (client, pid, ...?)
	// - dup2(child_stdin, in), dup2(child_stdout, out)
	// - this->cgis[out] = CGIProcess
	// - execve(client.cgi_path, envp)
	// throw if there's any issue
}
