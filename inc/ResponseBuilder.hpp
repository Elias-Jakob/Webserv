#ifndef RESPONSE_BUILDER_HPP
# define RESPONSE_BUILDER_HPP
# include <iostream>
# include "structs.h"
# include <map>

# include <iostream>
# include <string>
# include <map>
# include <algorithm>
# include "Get.hpp"
# include <sstream>
# include "HttpStatus.hpp"
# include "ConfigFileParser.hpp"
# include "HttpRequest.hpp"
/**
	* @class ResponseBuilder
	* @brief Builds the full Http-Response to a string.
*/
class ResponseBuilder
{
	public:
		ResponseBuilder();
		~ResponseBuilder();

		std::string response(t_executionResult result);
		std::string errorResponse(HttpRequest *request);
		std::string	errorResponseViaCode(int errorCode);
		std::string	errorResponseViaResult(t_executionResult result);
		std::string cgiResponse(const std::string &cgiBody);
		std::string	redirectResponse(
						t_executionResult *result, 
						const std::string &redirectURL);
		bool		setConfig(t_Configs *serverConfig);

	private:
		t_Configs	*_serverConfig;

		std::string setErrorResponseHeaders(size_t contentLength);
		std::string	setErrorStatusLine(int errorCode);
		
		std::string buildStatusLine(t_executionResult *result);
		std::string buildResponseHeaders(t_executionResult &result);
		std::string generateErrorPage(const std::string &code, 
										const std::string &phrase);
		std::string buildFullResponse(const std::string &statusLine,
										const std::string &messageHeaders,
										const std::string &resultBody);
		
										std::string getHttpDate();
		std::string convertContentLength(); // size_t -> string
		
		std::string	statusLine(t_RequestLine reqLine, const std::string &codeStr, const std::string &phrase);
		bool		isValidHttpVersion(const std::string &version);
		bool		availableErrorPage(int errorCode, std::map<int, std::string>::iterator *itErrorPage);
		std::string	buildBody(int errorCode, const std::string &codeStr, const std::string &phrase);
		std::string getErrorPage(std::map<int, std::string>::iterator itErrorPage);

};

#endif