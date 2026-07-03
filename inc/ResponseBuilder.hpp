#ifndef RESPONSE_BUILDER_HPP
# define RESPONSE_BUILDER_HPP
# include <iostream>
# include "structs.h"
# include <map>

# include <iostream>
# include <string>
# include <map>
# include <algorithm>

# include <sstream>
# include "HttpStatus.hpp"
# include "ConfigFileParser.hpp"

/**
	* @class ResponseBuilder
	* @brief Builds the full Http-Response to a string.
*/
class ResponseBuilder
{
	public:
		ResponseBuilder();
		~ResponseBuilder();

		std::string formatResponse(t_executionResult result);
		std::string	buildErrorResponse(int errorCode);
		bool		setConfig(t_Configs *serverConfig);
		std::string cgiFormation(const std::string &cgiBody);
		std::string	redirectResponse(t_executionResult *result, 
										const std::string &redirecURL);

	private:
		t_Configs	*_serverConfig;

		std::string setErrorResponseHeaders(size_t contentLength);
		std::string buildStatusLine(t_executionResult *result);
		std::string buildResponseHeaders(t_executionResult &result);
		std::string generateErrorPage(const std::string &code, 
										const std::string &phrase);
		std::string buildFullResponse(const std::string &statusLine,
										const std::string &messageHeaders,
										const std::string &resultBody);
		std::string getHttpDate();
};

#endif