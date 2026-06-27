#include "server.hpp"
#include "limits_defines.hpp"

void handleNewConnection(int listen_fd, std::vector<struct pollfd> &fds, std::map<int, ClientConnection> &clients, MethodExecuter &methodExecuter, ResponseBuilder &responseBuilder);
void handleClientRead(int client_fd, std::vector<struct pollfd> &fds, size_t index, std::map<int, ClientConnection> &clients);
void handleClientWrite(int client_fd, std::vector<struct pollfd> &fds, size_t index, std::map<int, ClientConnection> &clients);

int main(int argc, char *argv[])
{
// ConfigFileParser
	if (argc != 2)
		std::cout << "No configuration file provided" << std::endl;
	ConfigFileParser configFile;
	configFile.parseFile(argv[1]);
	t_Server	serverConfig;
	serverConfig = configFile.getServerConfigData();
	MethodExecuter	methodExecuter;
	ResponseBuilder	responseBuilder;
	methodExecuter.setConfig(&serverConfig);
	responseBuilder.setConfig(&serverConfig);
	std::cout << "ConfigFile server_name = " << serverConfig.serverName << std::endl;

	srand(rand());
// getaddr()
	struct addrinfo	*res;
	int				ret;
	ret = getaddrinfo("127.0.0.1", "8080", NULL, &res);
	if (ret != 0)
	{
		std::cout << "\033[31mError: getaddrinfo()\033[m" << std::endl;
		std::cout << gai_strerror(ret) << std::endl;
	}
// socket()
	int	sfd;
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
	{
		std::cout << "\033[31mError opening socket fd\033[m" << std::endl;
		perror("socket:");
	}
	std::cout << "\033[32msocket FD: " << sfd << "\033[m" << std::endl;
// bind()
	int opt = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		perror("setsockopt");
		freeaddrinfo(res);
		return 1;
	}
	if (bind(sfd, res->ai_addr, res->ai_addrlen) == -1)
	{
		std::cout << errno << std::endl;
		perror("bind:");
		close (sfd);
		freeaddrinfo(res);
		return 1;
	}
	std::cout << "\033[32mbind success...\033[m" << std::endl;
// listen()
	if (listen(sfd, 10) == -1)
	{
		perror("listen:");
		return 1;
	}
	std::cout << "\033[32mlisten() success...\033[m" << std::endl;
	fcntl(sfd, F_SETFL, O_NONBLOCK); // make it nonblocking
	// arr of fds to monitor
	std::vector<struct pollfd> fds;
	// listening socket
	struct pollfd pfd;
	pfd.fd = sfd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	fds.push_back(pfd);
	std::map<int, ClientConnection> clients;
	while (1)
	{
		int	ready = poll(&fds[0], fds.size(), -1); // wait for activity on any socket
		if (ready < 0)
		{
			perror("poll:");
			break ;
		}
		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents == 0)
				continue ;
			if (fds[i].fd == sfd) // -> new connection
			{
				handleNewConnection(sfd, fds, clients, methodExecuter, responseBuilder);
			}
			else if (fds[i].revents & POLLIN) // -> data to read
			{
				handleClientRead(fds[i].fd, fds, i, clients);
			}
			else if (fds[i].revents & POLLOUT) // ready to write
			{
				handleClientWrite(fds[i].fd, fds, i, clients);
			}
		}
	}
	return (0);
}

void handleNewConnection(int listen_fd, std::vector<struct pollfd>& fds, std::map<int, ClientConnection> &clients, MethodExecuter &methodExecuter, ResponseBuilder &responseBuilder)
{
	int client_fd = accept(listen_fd, NULL, NULL);
	if (client_fd < 0)
	{
		if (errno != EWOULDBLOCK)  // Non-blocking accept
			perror("accept");
		return ;
	}

	// Make client socket non-blocking
	fcntl(client_fd, F_SETFL, O_NONBLOCK);

	// Add to poll array
	struct pollfd pfd;
	pfd.fd = client_fd;
	pfd.events = POLLIN;  // Wait for data to read
	pfd.revents = 0;
	fds.push_back(pfd);

	// Initialize client connection directly in map (avoid copy issues)
	clients[client_fd].fd = client_fd;
	clients[client_fd].state = READING_REQUEST;
	clients[client_fd].request = new HttpRequest();
	clients[client_fd].executor = &methodExecuter;
	clients[client_fd].responseBuilder = &responseBuilder;
	clients[client_fd].bytes_sent = 0;
	
	std::cout << "New client: " << client_fd << std::endl;
}

void handleClientRead(int client_fd, std::vector<struct pollfd>& fds, size_t index, std::map<int, ClientConnection> &clients)
{
	std::cout << "ClientRead() for fd: " << client_fd << std::endl;
	char buffer[4096];
	ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);

	if (bytes < 0)
	{
		// Non-blocking socket: EAGAIN/EWOULDBLOCK means no data available yet
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			std::cout << "No data available yet (non-blocking)" << std::endl;
			return;  // Not an error, just try again later
		}
		// Real error
		std::cout << "recv() error: " << strerror(errno) << std::endl;
		close(client_fd);
		fds.erase(fds.begin() + index);
		clients.erase(client_fd);
		return;
	}
	else if (bytes == 0)
	{
		// Client closed connection
		std::cout << "Client closed connection" << std::endl;
		close(client_fd);
		fds.erase(fds.begin() + index);
		clients.erase(client_fd);
		return;
	}

	std::cout << "Received " << bytes << " bytes from " << client_fd << std::endl;

	if (clients[client_fd].request == NULL)// Check if request pointer is valid
	{
		std::cerr << "ERROR: request pointer is NULL!" << std::endl;
		close(client_fd);
		fds.erase(fds.begin() + index);
		clients.erase(client_fd);
		return;
	}

	// Parse request - use string constructor with length to avoid buffer overflow
	std::string request_data(buffer, bytes);
	clients[client_fd].request->parseRequest(request_data);
	if (clients[client_fd].request->parsingComplete())
	{
		clients[client_fd].state = PROCESSING;
		clients[client_fd].processRequest();
		clients[client_fd].state = SENDING_RESPONSE;

		fds[index].events = POLLOUT;
	}
}

void handleClientWrite(int client_fd, std::vector<struct pollfd>& fds, size_t index, std::map<int, ClientConnection> &clients)
{
	ssize_t		sent;

	std::cout << "\033[35m==========\nRESPONSE sending...\n" << std::endl;
	sent = send(client_fd, clients[client_fd].response_buffer.c_str(), clients[client_fd].response_buffer.size(), 0);

	if (sent < 0)
	{
		// Non-blocking socket: EAGAIN/EWOULDBLOCK means can't send right now
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			std::cout << "Socket not ready for writing, try again later" << std::endl;
			return;  // Keep connection open, try again on next POLLOUT
		}
		// Real error
		std::cout << "send() error: " << strerror(errno) << std::endl;
		close(client_fd);
		fds.erase(fds.begin() + index);
		clients.erase(client_fd);
		return;
	}
	
	std::cout << "bytes sent: " << sent << "\n==========\033[m" << std::endl;

	// Clean up: close connection and remove from tracking
	// if (clients[client_fd].keep_alive == false)
	// {
	close(client_fd);
	fds.erase(fds.begin() + index);
	clients.erase(client_fd);  // This will call destructor and free request/response
	// }
	// else
	// {
		// clients[client_fd].cleanUpClient();
		// fds[index].events = POLLIN;
	// }
}
