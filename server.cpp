#include "server.hpp"

void handleNewConnection(int listen_fd, std::vector<struct pollfd> &fds, std::map<int, ClientConnection> &clients);
void handleClientRead(int client_fd, std::vector<struct pollfd> &fds, size_t index, std::map<int, ClientConnection> &clients);
void handleClientWrite(int client_fd, std::vector<struct pollfd> &fds, size_t index, std::map<int, ClientConnection> &clients);

int main(void)
{
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
				handleNewConnection(sfd, fds, clients);
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


void handleNewConnection(int listen_fd, std::vector<struct pollfd>& fds, std::map<int, ClientConnection> &clients)
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
	clients[client_fd].response = NULL;
	clients[client_fd].bytes_sent = 0;
	
	std::cout << "New client: " << client_fd << std::endl;
}

void handleClientRead(int client_fd, std::vector<struct pollfd>& fds, size_t index, std::map<int, ClientConnection> &clients)
{
	std::cout << "ClientRead() for fd: " << client_fd << std::endl;
	char buffer[4096];
	ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);

	if (bytes <= 0) // Connection closed or error
	{
		std::cout << "Connection closed or error" << std::endl;
		close(client_fd);
		fds.erase(fds.begin() + index);
		clients.erase(client_fd);  // Clean up client data
		return;
	}
	buffer[bytes] = '\0';
	std::cout << "Received " << bytes << " bytes from " << client_fd << std::endl;
    
	// Check if request pointer is valid
	if (clients[client_fd].request == NULL)
	{
		std::cerr << "ERROR: request pointer is NULL!" << std::endl;
		close(client_fd);
		fds.erase(fds.begin() + index);
		clients.erase(client_fd);
		return;
	}

	// Parse request
	clients[client_fd].request->parseRequest(buffer);
	clients[client_fd].state = PROCESSING;
	// Build response (store in ClientConnection, not local variable!)
	clients[client_fd].response = new HttpResponse(clients[client_fd].request);
	clients[client_fd].state = SENDING_RESPONSE;
    
	// Switch to POLLOUT to send response
	fds[index].events = POLLOUT;
}

void handleClientWrite(int client_fd, std::vector<struct pollfd>& fds, size_t index, std::map<int, ClientConnection> &clients)
{
	ssize_t		sent;

	std::string status_line = clients[client_fd].response->getStatusLine();
	sent = send(client_fd, status_line.c_str(), status_line.size(), 0);
	std::cout << "\033[36m____________________\nRESPONSE sending...\n\033[35m" << status_line << std::endl;

	std::string message_headers = clients[client_fd].response->getMessageHeaders();
	sent += send(client_fd, message_headers.c_str(), message_headers.size(), 0);
	std::cout << message_headers << std::endl;

	std::string message_body = clients[client_fd].response->getMessageBody();
	sent += send(client_fd, message_body.c_str(), message_body.size(), 0);
	std::cout << message_body << "____________________\033[m"<< std::endl;
	// std::cout << "bytest sent" << sent << std::endl;
	if (sent < 0)
		perror("send");
    
	// Clean up: close connection and remove from tracking
	close(client_fd);
	fds.erase(fds.begin() + index);
	clients.erase(client_fd);  // This will call destructor and free request/response
}
// need a loop to receive and send messages
	// // accept()
	// while (1)
	// {
	// 	struct sockaddr_in	client_addr;
	// 	socklen_t			client_len = sizeof(client_addr);
	// 	int					client_fd = accept(sfd, (struct sockaddr*)&client_addr, &client_len);
	// 	HttpRequest			request;
	// 	if (client_fd == -1)
	// 	{
	// 		perror("accept:");
	// 		return -1;
	// 	}
	// 	else
	// 	{
	// 		std::cout << "\033[32mclient fd: \033[m" << client_fd << std::endl;
	// 	// recv()
	// 		char buffer_request[1024];
	// 		ssize_t m_len;
	// 		m_len = recv(client_fd, buffer_request, sizeof(buffer_request) -1, 0);
	// 		if (m_len > 0)
	// 		{
	// 			buffer_request[m_len] = '\0';
	// 			request.parseRequest(buffer_request);
	// 			HttpResponse	response(&request);
	// 			std::cout << "Status Line = " << response.getStatusLine() << std::endl;
	// 		// send()

	// 			std::string status_message = response.getStatusLine();
	// 			ssize_t		bytes_sent = send(client_fd, status_message.c_str(), status_message.size(), 0);
	// 			std::cout << "Header sent to client" << std::endl;
	// 			std::string message_headers = response.getMessageHeaders();
	// 			bytes_sent += send(client_fd, message_headers.c_str(), message_headers.size(), 0);

	// 			std::string message_body = "\r\n" + response.getMessageBody();
	// 			bytes_sent += send(client_fd, message_body.c_str(), message_body.size(), 0);
	// 			std::cout << "Body sent to client" << std::endl;
	// 			if (bytes_sent == -1)
	// 			{
	// 				perror("\033[31msend:\033[m");
	// 			}
	// 			else
	// 				std::cout << "\033[32msent message to client of " << bytes_sent << "\033[m" << std::endl;
	// 				// std::cout << "REQUEST FROM CLIENT:\n" << buffer_request << std::endl;
	// 		}
	// 		else
	// 			std::cout << "\033[31mno message received\033[m" << std::endl;
	// 	}
	// 	// sleep(1);
	// 	close(client_fd);
	// }

// // close and free
// 	close(sfd);
// 	freeaddrinfo(res);
// 	return (0);
// }