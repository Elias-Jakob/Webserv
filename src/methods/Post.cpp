#include "Post.hpp"

Post::Post() : AMethod()
{
}

Post::Post(std::string name) : AMethod()
{
	_AMethod = name;
	std::cout << "POST" << std::endl;
}

Post::~Post()
{}

/** 
	* check the content type and parse the body appropialtly
**/
bool Post::execute()
{
	std::cout << "POST->execute()" << std::endl;

	std::cout << "Content-Data = \n{" << std::endl;
	std::cout << "\tcontent-type: " << _contentData.type << std::endl;
	std::cout << "}" << std::endl;
	if (_contentData.type == "application")
	{
		appendToFile("form_input.txt");
	}
	else
	{
		std::cout << "No Post execution implemented!" << std::endl;
	}
	_code = "200";
	_phrase = "OK";
	std::cout << "POST->execute() end\n" << std::endl;
	return true;
}

void Post::appendToFile(std::string filename)
{
	std::ofstream	output(filename.c_str());
	std::map<std::string, s_FormField>::iterator it = _parsedBody.begin();
	std::map<std::string, s_FormField>::iterator ite = _parsedBody.end();

	while (it != ite)
	{
		output << it->first << " = " << it->second.value << std::endl;
		it++;
	}
	output.close();
}