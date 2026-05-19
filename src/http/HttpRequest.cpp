#include "HttpRequest.hpp"

HttpRequest::HttpRequest(){}

HttpRequest::~HttpRequest(){}

// Ignore empty lines before the request-line
void	ignoreEmptyLines(const std::string &input, size_t *pos)
{
	std::cout << "ignoreEmptyLines..." << std::endl; // debug
	size_t CR = 0;
	size_t LF = 1;
	while (input[CR] == '\r' && input[LF] == '\n')
	{
		CR += 2;
		LF += 2;
	}
	*pos = CR;
}

/*
Gets message from recv-buffer (client) and parses it into HttpRequest.
Split first line -> Request-Line (GET /index.html HTTP/1.1).
Rest are Request-Headers, map[key] = value => (Host: 127.0.0.1:8080)
*/
bool	HttpRequest::parseRequest(const std::string &input)
{
	std::cout << "\33[33m____________________\nHTTP_REQUEST received..." << std::endl;
	std::cout << "\33[35m" << input << std::endl;
	// std::cout << "\33[33m "<< "\";}" << std::endl;
	std::cout << "\33[m\33[33mPARSING request..." << std::endl;
	std::string	input_headers;
	size_t		currentPos = 0;
	
	ignoreEmptyLines(input, &currentPos); // ignore CRLF before request-line

	if (!parseRequestLine(input, &currentPos))
	{
		std::cerr << "ERROR: request-line" << std::endl;
		return false;
	}
	parseRequestHeaders(input, &currentPos);

	parseRequestBody(input, &currentPos);

	std::cout << "printing result..." << std::endl;
	printRequest();
	std::cout << "____________________\33[m\n" << std::endl;
	return true;
}

/* Extracts tokens of Request-Line and returns CRLF postition */
bool	HttpRequest::parseRequestLine(const std::string &input, size_t *currentPos)
{
	std::cout << "parseRequestLine..." << std::endl; // debug
	std::string	reqLine;
	size_t		posCRLF;
	size_t		posSP1;
	size_t		posSP2;

	posCRLF = input.find("\r\n", *currentPos);
	if (posCRLF == 0)
		return false;
	reqLine = input.substr(*currentPos, posCRLF - *currentPos);
	// std::cout << "(" << reqLine << ")" << std::endl;

	posSP1 = reqLine.find(' ', 0);
	// std::cout << "space 1 = " << posSP1 << std::endl;
	requestLine.method = reqLine.substr(0, posSP1); // segfault here

	posSP2 = reqLine.find(' ', posSP1 + 1);
	requestLine.requestURI = reqLine.substr(posSP1 + 1, posSP2 - (posSP1 + 1));

	requestLine.version = reqLine.substr(posSP2 + 1);
	*currentPos = posCRLF + 2;
	return true;
}

size_t skipLWS(std::string val, size_t start, size_t end)
{
	size_t cnt = 0;
	while (val[start] == ' ' && start < end)
	{
		cnt++;
		start++;
	}
	return (cnt);
}

std::vector<std::string> splitHeaderValByComma(std::string val)
{
	std::vector<std::string>	split;
	size_t	i = val.size();
	size_t	start = 0;
	size_t	end = 0;

	while(end < i)
	{
		end = val.find(',', start);
		size_t spaces = skipLWS(val, start, end);
		start+= spaces;
		if (end < i)
			split.push_back(val.substr(start, end - start));
		else if (end >= i)
		{
			split.push_back(val.substr(start, val.size()));
			break ;
		}
		start = end + 1;
	}
	return split;
}

/* Extracts Request-Headers and stores it split by ": " in map 
!!!!header-names shoult be lowercase -> case-insensitive
*/
bool HttpRequest::parseRequestHeaders(const std::string &input, size_t *pos)
{
	std::cout << "parseRequestHeaders..." << std::endl; // debug
	std::vector<std::string>	lines;
	std::istringstream			stream(input);
	std::string					line;
	size_t 						posColon;
	std::string 				key;
	std::string 				val;

	std::getline(stream, line); // Skip Request line
	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			break;
		lines.push_back(line);
		*pos += line.size() + 2; // for the '\r'
	}

	for (size_t i = 0; i < lines.size(); i++)
	{
		posColon = 0;
		posColon = lines[i].find(":", 0);
		if (posColon > 1 && posColon < lines[i].size())
		{
			key = lines[i].substr(0, posColon);
			val = lines[i].substr(posColon + 2); // logic to remove leading and trailing LWS

			requestHeaders[key] = splitHeaderValByComma(val);
		}
	}
	return true;
}

bool	HttpRequest::parseRequestBody(const std::string &input, size_t *pos)
{
	if (input[*pos] == '\r' && input[*pos + 1] == '\n')
		*pos += 2;
	requestBody = input.substr(*pos);
	return true;
}

void HttpRequest::printRequest(void)
{
	std::cout << "Request-Line:\n{" << std::endl;
	std::cout << "\tAMethod: [" << requestLine.method << "]" << std::endl;
	std::cout << "\tPath: [" << requestLine.requestURI << "]" << std::endl;
	std::cout << "\tVersion: [" << requestLine.version << "]" << std::endl;
	std::cout << "}" << std::endl;

	std::map<std::string, std::vector<std::string> >::iterator it = requestHeaders.begin();
	std::map<std::string, std::vector<std::string> >::iterator ite = requestHeaders.end();
	std::cout << "Request-Headers:\n{" << std::endl;
	while (it != ite)
	{
		std::cout << "\t [" << it->first << "] = {";
		for (size_t i = 0; i < it->second.size(); i++)
		{
			if (i < it->second.size() - 1)
				std::cout << "\"" << it->second[i] << "\", ";
			else
				std::cout << "\"" << it->second[i] << "\"";
		}
		std::cout << "}" << std::endl;
		it++;
	}
	std::cout << "}" << std::endl;
	std::cout << "Request-Body:\n{" << std::endl;
	std::cout << requestBody << "\n}" << std::endl;
}

s_RequestLine &HttpRequest::getRequestLine()
{
	return requestLine;
}

std::map<
			std::string,
			std::vector<std::string> > &HttpRequest::getRequestHeaders()
{
	return requestHeaders;
}

std::string	&HttpRequest::getRequestBody()
{
	return requestBody;
}



// split first by / and then by ;
// void splitHeaderValue(std::string value)
// {
// 	// std::cout << "splitHeaderValue()" << std::endl;
// 	// std::cout << value << std::endl;
// 	t_headerValue	headerVal;
// 	size_t			start = 0;
// 	size_t			end = 0;
// 	headerVal.priority = 1;
	
// 	end = value.find('/', 0);
// 	// std::cout << end << std::endl;
// 	if (end < value.size())
// 	{
// 		headerVal.type = value.substr(start, end - start);
// 		start = end + 1;
// 		size_t	quality;
// 		quality = value.find(';', start);
// 		if (quality < value.size())
// 		{
// 			headerVal.subtype = value.substr(start, quality - start);
// 			headerVal.param = value.substr(quality, value.size() - quality);
// 		}
// 		else
// 		{
// 			headerVal.subtype = value.substr(start, value.size());
// 		}
// 	std::cout << "headerVal =\n{" << std::endl;
// 	std::cout << "\ttype: " << headerVal.type;
// 	std::cout << "\n\tsubtype: " << headerVal.subtype;
// 	std::cout << "\n\tparam: " << headerVal.param;
// 	std::cout << "\n\tpriority: " << headerVal.priority << "\n}" << std::endl;
// 	}
// }

