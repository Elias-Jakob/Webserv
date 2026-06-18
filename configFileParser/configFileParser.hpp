#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

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

typedef struct s_Token
{
	e_TokenType	type;
	std::string	val;
}	t_Token;

typedef struct s_Location
{
	std::string		path;
	std::string		root;
	bool			redirect;
	std::string		redirectURL;
	int				redirectCode;
	bool			upload;
	std::string		uploadStore;
	bool			autoIndex; // directory listing
	std::string		defaultPage;
	std::vector<std::string>	allowedMethods;
	std::vector<std::string>	cgiExtensions;
}	t_Location;

typedef struct s_Server
{
	size_t						maxBodySize;
	std::string					serverName;
	std::vector<std::string>	listenInterfaces;
	std::map<int, std::string>	errorPages;
	std::vector<t_Location>		locations;

	std::map<std::string, std::vector<std::string> >	endpoints;
}	t_Server;

class ConfigFileParser
{
	public:
		ConfigFileParser();
		~ConfigFileParser();

		void parseFile(const std::string &filePath);

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
		t_Server	_server;

		void 	parseToDataStructure();
		size_t	createServer(size_t *i);
		// size_t	createLocation(size_t i);
		bool	checkIdentifier(const std::string identifier);
		void	setValue(const std::string id, size_t j);
		void	printServer();
		size_t	convertStrToSize(const std::string value);
		
		size_t	createLocation(size_t i);
		size_t	setLocationVal(size_t i, t_Location *loc);

		void	parseEndpoints();
};

/*
	TOKENS:
		KEYWORD("server"),
		BRACE_OPEN,
		BRACE_CLOSE,
		IDENTIFIER("listen"),
		ASSIGN,
		VALUE("8080"),
		END_OF_FILE
*/