# include "utils.hpp"
# include <vector>

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
	}
	else result = "Unknown AF";
	return (result);
}

/**
	* @brief lexically resolves '.' and '..' segments of path (no filesystem
	*	access, no symlink resolution) as a realpath()-free alternative.
	* @return the normalized path, with no leading/trailing slash
	*	(empty string means "no segments left", i.e. the root itself).
*/
std::string	utils::normalizePath(const std::string &path)
{
	std::vector<std::string>	stack;
	std::stringstream			ss(path);
	std::string					segment;

	while (std::getline(ss, segment, '/')) {
		if (segment.empty() || segment == ".")
			continue;
		if (segment == "..") {
			if (!stack.empty())
				stack.pop_back();
			continue;
		}
		stack.push_back(segment);
	}
	std::string	result;
	for (size_t i = 0; i < stack.size(); i++) {
		if (i > 0)
			result += "/";
		result += stack[i];
	}
	return (result);
}
