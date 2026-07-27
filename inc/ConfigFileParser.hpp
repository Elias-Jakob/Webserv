#ifndef CONFIGFILEPARSER_HPP
# define CONFIGFILEPARSER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
# include <stdexcept>
# include <cerrno>
# include <cstring>
# include "print_controls.hpp"

enum e_TokenType
{
	SERVER,
	LOCATION,
	BRACE_OPEN,
	BRACE_CLOSE,
	IDENTIFIER,
	ASSIGN,
	VALUE,
	COMMA,
	PATH_LOCATION,
	NUMBER,
	STR,
	END_OF_FILE
};

typedef std::map<std::string, std::vector<std::string> >	t_MultiStrMap;

typedef struct s_Token
{
	e_TokenType	type;
	std::string	val;
}	t_Token;

typedef struct s_Location
{
	std::string		path;
	std::string		alias; // before: root
	bool			sizeIsSet;
	size_t			maxBodySize;
	bool			redirect;
	std::string		redirectURL;
	int				redirectCode;
	bool			upload;
	std::string		uploadStore;
	bool			autoIndex; // directory listing
	std::string		defaultPage;
	bool			formSubmit;
	std::string		formUploadFile;
	bool			cgi;
	std::string		cgiPath;
	std::vector<std::string>	allowedMethods;
	std::vector<std::string>	cgiExtensions;
	std::vector<std::string>	uploadExtensions; // .txt, .pdf, .jpg, ...
}				t_Location;

typedef struct s_Configs
{
	size_t						maxBodySize;
	std::string					serverName;
	std::string					root;
	std::vector<std::string>	listenInterfaces;
	std::map<int, std::string>	errorPages;
	std::vector<t_Location>		locations;

	t_MultiStrMap	endpoints;
}	t_Configs;

class ConfigFileParser
{
	public:
		ConfigFileParser();
		~ConfigFileParser();

		std::vector<t_Configs>	&parseFile(const std::string &filePath);
	private:
		// TOKENIZATION
		std::vector<t_Token>	_tokens;
		
		void 		tokenize(const std::string &input);
		bool		isValidChar(char c);
		e_TokenType	getTokenType(std::string tokenStr);
		std::string printTokenType(e_TokenType type);
		void		adjustTokens();
		void		printTokens();
		bool		isNbr(const std::string &s);

		// PARSING
		t_Configs	_configs;
		std::vector<t_Configs>	_servers;
		void 	parseToDataStructure();
		size_t	createServer(size_t *i);
		// size_t	createLocation(size_t i);
		bool	checkIdentifier(const std::string identifier);
		void	setValue(const std::string id, size_t j, t_Configs *serverConfigs);
		void	printServer(size_t z);
		size_t	convertStrToSize(const std::string value);
		
		size_t	createLocation(size_t i, t_Configs *serverConfigs);
		size_t	setLocationVal(size_t i, t_Location *loc);

		void	parseEndpoints(t_Configs *serverConf);
		void	printServers();

		// IPv6
		void	parseIPv6(t_Configs *serverConfs, const std::string &listenInterface);
		bool	isIPv6(const std::string &listenInterface);
};

#endif
