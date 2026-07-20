# include "cgi.hpp"

char	**cgi::getArray(std::vector<std::string> &lst)
{
	char	**arr = NULL;

	if (lst.empty())
		return (NULL);
	arr = new char*[lst.size() + 1]();
	for (size_t	i = 0; i < lst.size(); ++i)
		arr[i] = (char *)lst[i].c_str();
	return (arr);
}

std::string	cgi::strJoin(std::vector<std::string> &vec)
{
	std::string	concat;

	for (std::vector<std::string>::const_iterator	it = vec.begin();
			it != vec.end(); ++it) {
		if (it != vec.begin())
			concat += ",";
		concat += *it;
	}
	return (concat);
}

std::string	cgi::toMetaFormat(std::string	originalKey)
{
	for (std::string::iterator	it = originalKey.begin(); it != originalKey.end(); ++it) {
		*it = std::toupper(static_cast<unsigned char>(*it));
		if (*it == '-')
			*it = '_';
	}
	return ("HTTP_" + originalKey);
}

/**
	* @brief the only reason for cgi::tolower to exist is because std::tolower is not compatible with std::transform
*/
unsigned char	cgi::tolower(unsigned char c)
{
	if (c >= 'A' && c <= 'Z')
		return (c + ('a' - 'A'));
	return (c);
}

/**
	* @brief Search's through the cgi's output for any HTTP header's
	* @param body is the whole output returned by the cgi script
	* @param header is the string searched for in the cgiBody; it is expected in lower case
*/
std::string	cgi::checkForHeaders(std::string &body, const std::string &header)
{
	std::string	searchableBody(body);
	size_t	found, foundEndl;
	std::string	retHeader;

	std::transform(searchableBody.begin(), searchableBody.end(), searchableBody.begin(),
								cgi::tolower);
	found = searchableBody.find(header);
	if (found == std::string::npos)
		return ("");
	foundEndl = body.find("\n", found);
	if (foundEndl != std::string::npos)
		foundEndl++;
	retHeader = body.substr(found, foundEndl);
	body.erase(found, foundEndl);
	return (retHeader);
}
