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
// # include "MethodExecuter.hpp"

typedef struct	s_query
{
	std::string	key;
	std::string	value;
}				t_query;

typedef struct s_RequestLine
{
	std::string				method;
	std::string 			requestURI;
	std::string				version;
	std::string				queryStr;
	std::vector<t_query>	query;
}				t_RequestLine;

enum e_parsingState
{
	PARSING_REQUEST_LINE,
	PARSING_HEADERS,
	PARSING_BODY,
	PARSING_COMPLETE,
	PARSING_ERROR
};

/**
	* @class HttpRequest // HttpRequestParser
	* @brief parses the raw http-message and splits/stores it into the 
	*  data structure.
*/
class HttpRequest
{
	private:
		e_parsingState	_state;
		size_t			_current_pos;
		std::string		_messageBuffer;
		std::string		_fullMessageBody;

		ABodyParser		*_bodyParser;
		t_ContentData	_contentData;
		// PARSED DATA
		t_RequestLine	_requestLine;
		std::map<std::string, std::vector<std::string> >	_headers;
		std::map<std::string, s_FormField> _parsedMessageBody;
		// ERROR
		int				_errorCode;
		std::string		_fileExtension;
		std::string		_host;
		// SERVER CONFIGURATIONS
		std::vector<t_Configs>	_serverConfigs;
		t_Location		*_locationObj;
		std::string		_listeningInterface;
		HttpRequest(const HttpRequest &other);
		HttpRequest &operator=(const HttpRequest &other);
		
		bool	setErrorCode(int code);
		bool	isImplementedMethod();
		bool	isHttpVersionSupported();
		bool	foundEndOfRequest();
		void	addHeader(const std::string &key, const std::string &value);
		bool	isValidURI(const std::string &uri);
		std::string	toLowerCase(std::string &str);

		bool	hasQuery(size_t	*posQuery);
		void	handleQuery(size_t posQuery);
		void	setQueryPairs(const std::string &queryStr);
		void	setQueryKeyValue(const std::string &queryStr, size_t start, size_t posEqual, size_t end);
		void	extractFileExtension();
		
		// CHUNKED ENDODING
		bool	unchunkBody(); // new
		size_t	chunkedSize();
		std::string	chunkedData(size_t chunked_size);

		// SERVER LOCATION_OBJ
		void	findLocation();
		bool	isListeningTo(size_t i, const std::string &host);
		std::vector<std::string>	splitPath(const std::string &path);
		

	protected:
		// HELPERS
		bool	parseRequestLine();
		bool	parseHeaderLine();
		bool	extractBody();
		bool	createBodyParser();

		// BODY PARSING
		std::string	parseContentType(std::vector<std::string> value);
		ABodyParser *createMultiParser();
		ABodyParser *createFormParser();

	public:
		HttpRequest();
		~HttpRequest();

		// METHODS
		bool	parseRequest(const std::string &partialMessage);
		bool 	parsingComplete();
		bool	keepConnectionAlive();
		bool 	validRequest();

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

		bool								hasBodyContentLength();
		void	setServerConfigs(std::vector<t_Configs> _serverConfigs, const std::string &listeningInterface);
		// OUTPUT
		void	printRequest(void);
};

#endif
