#include "FormParser.hpp"

FormParser::FormParser()
{
	std::cout << "FormParser constructed" << std::endl;
}

FormParser::~FormParser(){}

/*
implement the encoding (URL-decode)
*/
bool FormParser::parse(std::string &str)
{
	std::cout << "FormParser::parse()\n\tstr: " << str << std::endl;
	if (_contentData.subtype == "octet-stream") {
		std::cout << "\toctet" << std::endl;
		s_FormField field;
		std::string value = str;
		urlDecode(value);
		field.value = value;
		_result[""] = field;
		
		std::map<std::string, s_FormField>::iterator it = _result.begin();
		std::map<std::string, s_FormField>::iterator ite = _result.end();
		while (it != ite) {
			std::cout << "it->first: " << it->first << " -> "<< it->second.value << std::endl;
			it++;
		}
	}
	else {
	std::vector<std::string> splittedStrings;
	size_t	start = 0;
	size_t	end = 0;

	while ((end = str.find('&', start)) < str.size())
	{
		splittedStrings.push_back(str.substr(start, end));
		start = end + 1;
	}
	if (end > start)
		splittedStrings.push_back(str.substr(start, end));

	std::string	key;
	std::string	value;
	for (size_t i = 0; i < splittedStrings.size(); i++)
	{
		end = 0;
		start = 0;
		end = splittedStrings[i].find('=', start);
		if (end < splittedStrings[i].size() && end > start)
		{
			key = splittedStrings[i].substr(start, end);
			value = splittedStrings[i].substr(end + 1, splittedStrings[i].size());
			urlDecode(key);
			urlDecode(value);
			s_FormField	field;
			field.value = value;
			_result[key] = field;
		}
	}
	}
	return true;
}

void FormParser::appendToFile(std::string filename)
{
	std::ofstream	output(filename.c_str());
	std::map<std::string, s_FormField>::iterator it = _result.begin();
	std::map<std::string, s_FormField>::iterator ite = _result.end();

	while (it != ite)
	{
		output << it->first << " = " << it->second.value << std::endl;
		it++;
	}
	output.close();
}


void FormParser::urlDecode(std::string &data)
{
    std::string result;

    for (size_t i = 0; i < data.length(); ++i)
	{
        if (data[i] == '+')
		{
            result += ' ';
        }
		else if (data[i] == '%' && i + 2 < data.length())
		{
			std::string hex = data.substr(i + 1, 2);
			result += (char)std::strtol(hex.c_str(), NULL, 16);
			// std::string hex = data.substr(data[i+1], data[i+2]);
			// std::cout << "hex" << hex << std::endl;
			// char *end;
			// result += std::strtod(hex.c_str(), &end);
            i += 2;
        }
		else
		{
            result += data[i];
        }
    }
    data = result;
}