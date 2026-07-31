#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

# include <iostream>
# include <string>
# include <map>
# include <algorithm>

# include <sstream>
# include <vector>
# include "limits_defines.hpp"
# include "ABodyParser.hpp"
# include "FormParser.hpp"
# include "MultipartParser.hpp"
# include "structs.h"
# include "print_controls.hpp"
# include "ConfigFileParser.hpp"

# include "RequestLineParser.hpp"
# include "RequestHeadsParser.hpp"
# include "RequestBodyParser.hpp"

/**
	* @class HttpRequest // HttpRequestParser
	* @brief parses the raw http-message and splits/stores it into the 
	*  data structure.
*/
class HttpRequest
{
	public:
		HttpRequest();
		~HttpRequest();

		// METHODS
		bool	parseRequest(const std::string &partialMessage, size_t bytesRecv);
		bool 	parsingComplete();
		bool	keepConnectionAlive();
		bool 	validRequest();
		void	setServerConfigs(std::vector<t_Configs> _serverConfigs, const std::string &listeningInterface);

		// GETTERS
		s_RequestLine						&getRequestLine();
		std::map<
			std::string,
			std::vector<std::string> >		&getRequestHeaders();
		std::string							&getRequestBody();
		std::map<std::string, s_FormField>	&getParsedBody();
		t_ContentData						&getContentData();
		std::string							&getMethod();
		int									getErrorCode();
		std::string							&getURI();
		std::string							&getHost();
		std::string							getRedirectLocation();
		std::string							getFileExtension();
		std::string							&getScriptName();
		std::string							&getPathInfo();
		t_Location							*getLocationObj();
		std::string							&getPathTranslated();
		bool								hasBodyContentLength();
		
		// OUTPUT
		void	printRequest(void);

	private:
		t_RequestData		data;
		RequestLineParser	*requestLineParser;
		RequestHeadsParser	*requestHeadsParser;
		RequestBodyParser	*requestBodyParser;
		ABodyParser			*_bodyParser;

		HttpRequest(const HttpRequest &other);
		HttpRequest &operator=(const HttpRequest &other);

		// CHECKS
		bool	isImplementedMethod();
		bool	isHttpVersionSupported();
		bool	isValidURI(const std::string &uri);
		bool	isValidHost();
		bool	isValidPostRequest();
		bool	isAllowedMethod();

		// HELPER
		void	adjustCurrentPos(size_t pos);
		void	setCurrentPos(size_t pos);
		bool	setErrorCode(int code);
		bool	foundEndOfRequest();
};

#endif
