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

class ResponseBuilder
{
// <<<<<<< HEAD:inc/ResponseBuilder.hpp
// 	private:
// 		std::string setErrorResponseHeaders(size_t contentLength);
// 		std::string setResponseHeaders(t_executionResult &result);
// 		std::string generateErrorPage(const std::string &code, const std::string &phrase);
//
// =======
// >>>>>>> origin/configFileParser:src/http/ResponseBuilder.hpp
	public:
		ResponseBuilder();
		~ResponseBuilder();

		std::string formatResponse(t_executionResult result);
		std::string	redirectResponse(t_executionResult *result, const std::string &redirecURL);
		std::string	buildErrorResponse(int errorCode);
		bool		setConfig(t_Configs *serverConfig);

	private:
		t_Configs	*_serverConfig;
		std::string setErrorResponseHeaders(size_t contentLength);
		std::string buildStatusLine(t_executionResult *result);
		std::string buildResponseHeaders(t_executionResult &result);
		std::string buildFullResponse(const std::string &statusLine,
										const std::string &messageHeaders,
										const std::string &resultBody);
		std::string generateErrorPage(const std::string &code, const std::string &phrase);
};

#endif
