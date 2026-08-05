#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <sstream>

# include <sys/socket.h>
# include <netinet/in.h>

namespace utils {
	unsigned char	tolower(unsigned char c);
	int	strToInt(const std::string &str);
	template <typename T>
	std::string	numToStr(const T &num)
	{
		std::stringstream	ss;

		ss << num;
		return (ss.str());
	}
	std::string	addrToStr(struct sockaddr addr);
	std::string	normalizePath(const std::string &path);
}

#endif // !UTILS_HPP
