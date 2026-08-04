# include "utils.hpp"

unsigned char	utils::tolower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return (c + ('a' - 'A'));
	return (c);
}

int	utils::strToInt(const std::string &str)
{
	int	result;
	std::stringstream	convert(str);

	if (!(convert >> result))
		result = 0;
	return (result);
}

std::string	utils::addrToStr(struct sockaddr addr)
{
	std::string	result;

	if (addr.sa_family == AF_INET) {
		struct sockaddr_in	*addr_in = reinterpret_cast<struct sockaddr_in *>(&addr);
		uint32_t	ip = ntohl(addr_in->sin_addr.s_addr);
		uint16_t	port = ntohs(addr_in->sin_port);
		result = utils::numToStr<uint32_t>((ip >> 24) & 0xFF) + "." +
			utils::numToStr<uint32_t>((ip >> 16) & 0xFF) + "." +
			utils::numToStr<uint32_t>((ip >> 8) & 0xFF) + "." +
			utils::numToStr<uint32_t>(ip & 0xFF) + ":" +
			utils::numToStr<uint16_t>(port);
		// TODO: check if the port portion is correct
	}
	// TODO: support ipv6
	// else if (addr.sa_family == AF_INET6)
	else result = "Unknown AF";
	return (result);
}
