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
