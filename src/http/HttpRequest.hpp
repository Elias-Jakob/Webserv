#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

# include <iostream>
# include <string>
# include <map>
# include <algorithm>

# include <sstream>
# include <vector>
# include "../../limits_defines.hpp"
# include "../parsers/ABodyParser.hpp"
# include "../parsers/FormParser.hpp"
# include "../parsers/MultipartParser.hpp"
# include "../../structs.h"

typedef struct s_RequestLine
{
	std::string	method;
	std::string requestURI;
	std::string	version;
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

		HttpRequest(const HttpRequest &other);
		HttpRequest &operator=(const HttpRequest &other);
		bool	setErrorCode(int code);
		bool	isValidMethod();
		bool	isHttpVersionSupported();
		bool	foundEndOfRequest();

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
		s_RequestLine	&getRequestLine();
		std::map<
			std::string,
			std::vector<std::string> >	&getRequestHeaders();
		std::string		&getRequestBody();
		std::map<std::string, s_FormField>	&getParsedBody();
		t_ContentData	&getContentData();
		std::string		&getMethod();
		int				getErrorCode();
		std::string		&getURI();

		// OUTPUT
		void	printRequest(void);
};

#endif