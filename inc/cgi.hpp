#ifndef CGI_HPP
# define CGI_HPP

# include <vector>
# include <string>
# include <algorithm>

namespace cgi {
	char	**getArray(std::vector<std::string> &lst);
	std::string	toMetaFormat(std::string	originalKey);
	std::string	strJoin(std::vector<std::string> &vec);
	std::string	checkForHeaders(std::string &body, const std::string &header);
}

#endif // !CGI_HPP
