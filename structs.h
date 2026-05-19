#ifndef STRUCTS_H
# define STRUCTS_H
# include <iostream>

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

#endif