# include "Server.hpp"
# include <arpa/inet.h>

void	printSocketInfo(int sockfd)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    if (getsockname(sockfd, reinterpret_cast<sockaddr*>(&addr), &len) == -1)
    {
				std::cout << "Helper function: " << std::strerror(errno) << std::endl;
        return;
    }

    char ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip)) == NULL)
    {
				std::cout << "Helper function: " << std::strerror(errno) << std::endl;
        return;
    }

    std::cout << "Local address: "
              << ip
              << ":"
              << ntohs(addr.sin_port)
              << std::endl;
}
