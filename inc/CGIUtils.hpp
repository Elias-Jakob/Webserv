#ifndef CGIUTILS_HPP
# define CGIUTILS_HPP

# include <vector>
# include <string>
# include <algorithm>

namespace cgi {
	char	**getArray(std::vector<std::string> &lst);
	std::string	toMetaFormat(std::string	originalKey);
	std::string	strJoin(std::vector<std::string> &vec);
	unsigned char	tolower(unsigned char c);
	std::string	checkForHeaders(std::string &body, const std::string &header);
}

#endif // !CGIUTILS_HPP
