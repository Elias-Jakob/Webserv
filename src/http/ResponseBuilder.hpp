#ifndef RESPONSE_BUILDER_HPP
# define RESPONSE_BUILDER_HPP
# include <iostream>
# include "../../structs.h"
# include <map>

# include <iostream>
# include <string>
# include <map>
# include <algorithm>

# include <sstream>
# include "HttpStatus.hpp"
# include "../config/ConfigFileParser.hpp"

class ResponseBuilder
{
	public:
		ResponseBuilder();
		~ResponseBuilder();

		std::string formatResponse(t_executionResult result);
		std::string	buildErrorResponse(int errorCode);
		bool		setConfig(t_Server *serverConfig);

	private:
		t_Server	*_serverConfig;
		std::string setErrorResponseHeaders(size_t contentLength);
		std::string setResponseHeaders(t_executionResult &result);
		std::string generateErrorPage(const std::string &code, const std::string &phrase);
};

#endif