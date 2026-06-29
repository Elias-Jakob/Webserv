#ifndef STRUCTS_H
# define STRUCTS_H
# include <iostream>
# include <map>
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
	std::string	statusCode;
    std::string statusPhrase;   // "OK", "Not Found", etc.
    std::string body;           // Response body content
    std::string contentType;    // "text/html", "application/json", etc.
	bool		keep_alive;
	std::string	lastModified;
	std::string	etag;
}				t_executionResult;

#endif