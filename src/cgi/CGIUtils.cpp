# include "CGIProcessLauncher.hpp"

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


