#include "FormParser.hpp"

FormParser::FormParser()
{
}

FormParser::~FormParser(){}

/*
implement the encoding (URL-decode)
*/
bool FormParser::parse(std::string &str)
{
	if (_contentData.subtype == "octet-stream") {
		s_FormField field;
		std::string value = str;
		urlDecode(value);
		field.value = value;
		_result[""] = field;
		
		std::map<std::string, s_FormField>::iterator it = _result.begin();
		std::map<std::string, s_FormField>::iterator ite = _result.end();
		while (it != ite) {
			it++;
		}
	}
	else {
		std::vector<std::string> splittedStrings;
		size_t	start = 0;
		size_t	end = 0;
		while ((end = str.find('&', start)) < str.size()) {
			splittedStrings.push_back(str.substr(start, end));
			start = end + 1;
		}
		if (end > start)
			splittedStrings.push_back(str.substr(start, end));
		std::string	key;
		std::string	value;
		for (size_t i = 0; i < splittedStrings.size(); i++) {
			end = 0;
			start = 0;
			end = splittedStrings[i].find('=', start);
			if (end < splittedStrings[i].size() && end > start) {
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

    for (size_t i = 0; i < data.length(); ++i) {
        if (data[i] == '+')
            result += ' ';
		else if (data[i] == '%' && i + 2 < data.length()) {
			std::string hex = data.substr(i + 1, 2);
			result += (char)std::strtol(hex.c_str(), NULL, 16);
            i += 2;
        }
		else
            result += data[i];
    }
    data = result;
}