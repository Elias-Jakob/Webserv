#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <sstream>

// WARNING: check if it's still alright to implement templated functions in the header. like here with numToStr
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
}

#endif // !UTILS_HPP
