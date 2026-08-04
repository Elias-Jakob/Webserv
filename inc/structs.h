#ifndef STRUCTS_H
# define STRUCTS_H
# include <iostream>
# include <map>

#include "ConfigFileParser.hpp"

typedef struct s_FormField
{
	std::string	value;
	std::string	filename;
	std::string contentType;
	bool		isFile;
}	t_FormField;

typedef struct s_ContentData
{
	std::string type;
	std::string subtype;
	std::string boundary;
}				t_ContentData;

typedef struct s_headerValue
{
	std::string			value;
	std::map<
		std::string,
		std::string>	params;
}						t_headerValue;

typedef struct	s_node
{
	std::string type;
	s_node	*l;
	s_node	*r;
}				t_node;

typedef struct	s_token
{
	std::string	type;
	size_t		start;
	size_t		end;
	size_t		posDel;
}				t_token;

typedef struct s_executionResult
{
    bool        success;        // Did execution succeed?
	std::string	HttpVersion;
	std::string	statusCode;
    std::string statusPhrase;   // "OK", "Not Found", etc.
    std::string body;           // Response body content
    std::string contentType;    // "text/html", "application/json", etc.
	bool		keep_alive;
	std::string	lastModified;
	std::string	etag;
	std::string	uploadedLocation;
}				t_executionResult;

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

typedef struct	s_RequestData
{
		// t_RequestData	_data;
		e_parsingState	_state;
		size_t			_current_pos;
		std::string		_messageBuffer;
		std::string		_fullMessageBody;

		// ABodyParser		*_bodyParser;
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
		std::string		_scriptName;
		std::string		_pathInfo;
		std::string		_pathTranslated;
}				t_RequestData;


#endif