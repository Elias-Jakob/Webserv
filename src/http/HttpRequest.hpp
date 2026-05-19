#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

# include <iostream>
# include <string>
# include <map>
# include <algorithm>

# include <sstream>
# include <vector>

typedef struct s_RequestLine
{
	std::string	method;
	std::string requestURI;
	std::string	version;
}				t_RequestLine;

class HttpRequest
{
	private:
		t_RequestLine					requestLine;
		std::map<
			std::string,
			std::vector<std::string> >	requestHeaders;
		std::string						requestBody;

		HttpRequest(const HttpRequest &other);
		HttpRequest &operator=(const HttpRequest &other);

	protected:
		bool	parseRequestLine(const std::string &input, size_t *currentPos);
		bool	parseRequestHeaders(const std::string &input, size_t *currentPos);
		bool	parseRequestBody(const std::string &input, size_t *currentPos);

	public:
		HttpRequest();
		~HttpRequest();

		void	printRequest(void);
		bool	parseRequest(const std::string &input);
		s_RequestLine	&getRequestLine();
		std::map<
			std::string,
			std::vector<std::string> > &getRequestHeaders();
		std::string		&getRequestBody();
};

#endif
/* Parse header-values as simple strings, because we only need certain headers
completely parsed for some AMethods. So it is faster not to parse everything
by default.
*/

// AMethods = GET, POST, DELETE
// typedef struct s_headerValue
// {
// 	std::string	type;
// 	std::string subtype;
// 	std::string param;
// 	float		priority;
// }				t_headerValue;