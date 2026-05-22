#include "MultipartParser.hpp"

MultipartParser::MultipartParser()
{
    std::cout << "MultipartParser constructed" << std::endl;
}

MultipartParser::~MultipartParser()
{}

/**
	* Helper: Trim whitespace from both ends
**/
static std::string trim(const std::string &str)
{
	size_t start = 0;
	size_t end = str.length();
	
	while (start < end && std::isspace(str[start]))
		start++;
	while (end > start && std::isspace(str[end - 1]))
		end--;
	
	return str.substr(start, end - start);
}

/**
	*Remove surrounding quotes from a string
**/
static std::string unquote(const std::string &str)
{
	std::string trimmed = trim(str);
	if (trimmed.length() >= 2 && trimmed[0] == '"' && trimmed[trimmed.length() - 1] == '"')
		return trimmed.substr(1, trimmed.length() - 2);
	return trimmed;
}

/**
	* extracts header-name field.
	* RETURNS false, if no name<->value delimiter(':') found.
**/
bool extractHeaderName(const std::string &line, std::string &headerName, std::string &rest)
{
	size_t colonPos;

	colonPos = line.find(':');
	if (colonPos == std::string::npos)
		return false;

	headerName = trim(line.substr(0, colonPos));
	rest = trim(line.substr(colonPos + 1));
	return true;
}

/**
	* exctract and returns header-value.
	* header-value could be string or parameter!
**/
std::string	extractHeaderValue(std::string &rest, size_t *pos)
{
	size_t semiPos;
	std::string part;

	semiPos = rest.find(';', *pos);
	if (semiPos == std::string::npos)
	{
		part = trim(rest.substr(*pos));
		*pos = rest.length();
	}
	else
	{
		part = trim(rest.substr(*pos, semiPos - *pos));
		*pos = semiPos + 1;
	}
	return part;
}

bool	setHeaderValParameter(std::map<std::string, std::string> &params, std::string &part)
{			
	size_t 		eqPos;
	std::string	key;
	std::string	value;

	eqPos = part.find('=');
	if (eqPos != std::string::npos)
	{
		key = trim(part.substr(0, eqPos));
		value = unquote(part.substr(eqPos + 1));
		params[key] = value;
	}
	return true;
}

// Parse a single header line into name, value, and parameters
// Example: "Content-Disposition: form-data; name="file"; filename="test.txt""
// Returns: headerName="Content-Disposition", headerValue="form-data"
//          params["name"]="file", params["filename"]="test.txt"
static void parseHeaderLine(const std::string &line, std::string &headerName, 
                           std::string &headerValue, std::map<std::string, std::string> &params)
{
	std::cout << "parseHeaderLine()..." << std::endl;
	std::string	rest;
	if (!extractHeaderName(line, headerName, rest))
		return ;

	size_t pos = 0;
	bool first = true;
	while (pos < rest.length()) // get parameter->attributes
	{
		std::string	part = extractHeaderValue(rest, &pos);
		if (first) // set headerValue
		{
			headerValue = part;
			first = false;
		}
		else // set headerVal-parameter
		{
			// setHeaderValParameter(params, rest); // why does this funtion not work???
			// Remaining parts are key=value parameters
			size_t eqPos = part.find('=');
			if (eqPos != std::string::npos)
			{
				std::string key = trim(part.substr(0, eqPos));
				std::string value = unquote(part.substr(eqPos + 1));
				params[key] = value;
			}
		}
	}
}

/**
	* extracts header-line (delimiter = "\r\n").
	* adjusts position of current index in whole header string.
**/
static std::string	extractHeaderLineAdjustPos(const std::string &rawHeaders, size_t *pos)
{
	std::string line;
	size_t lineEnd;

	lineEnd = rawHeaders.find("\r\n", *pos);
	if (lineEnd == std::string::npos)
	{
		line = rawHeaders.substr(*pos);
		*pos = rawHeaders.length();
	}
	else
	{
		line = rawHeaders.substr(*pos, lineEnd - *pos);
		*pos = lineEnd + 2; // Skip \r\n
	}
	return line;
}

/**
	* Parse all headers from a headers section
	* Input: "Content-Disposition: form-data; name="file"\r\nContent-Type: text/plain"
**/
static std::map<std::string, std::map<std::string, std::string> > parseHeaders(const std::string &rawHeaders)
{
	std::cout << "parseHeaders()..." << std::endl;
	std::map<std::string, std::map<std::string, std::string> > headers;
	size_t		pos = 0;

	while (pos < rawHeaders.length())
	{
		std::string line = extractHeaderLineAdjustPos(rawHeaders, &pos);
		if (line.empty())
			continue;
		std::string headerName;
		std::string	headerValue;
		std::map<std::string, std::string> params;
		parseHeaderLine(line, headerName, headerValue, params);
		if (!headerName.empty()) // set header values;
		{
			// Store header value as "__value" key
			params["__value"] = headerValue;
			headers[headerName] = params;
			std::cout << "Header: " << headerName << " = " << headerValue << std::endl;
			for (std::map<std::string, std::string>::iterator it = params.begin(); 
			     it != params.end(); ++it)
			{
				if (it->first != "__value")
					std::cout << "  " << it->first << " = \"" << it->second << "\"" << std::endl;
			}
		}
	}
	return headers;
}

/**
    * Parses body sent by a POST-Request of "Content-Type: multipart/form-data".
	* 
**/
bool MultipartParser::parse(std::string &body)
{
	std::cout << "parsing MultipartParser..." << std::endl;
	std::string fullBoundary = "--" + _contentData.boundary;
	size_t partStart = body.find(fullBoundary);
	if (!findFirstBoundary(body, &partStart, fullBoundary))
		return false;

	while (partStart < body.length()) // Process each part
	{
		s_extractedData	data;
		size_t			nextBoundary;
		if (!extractHeadersAndContent(data, body, partStart, fullBoundary, &nextBoundary))
			break ;
		if (data.headers.count("Content-Disposition") > 0)
			createFormField(data);
		partStart = nextBoundary + 2 + fullBoundary.length(); // move to next part
		if (partStart + 2 <= body.length() && body.substr(partStart, 2) == "--") // Check if this is the final boundary (ends with --)
			break ;
		if (partStart < body.length() && body.substr(partStart, 2) == "\r\n")
			partStart += 2;
	}
	printParsedResult();
	return true;
}

/**
	* adjusts the starting point after the boundary
	* RETURN false if no boundary found
**/
bool	MultipartParser::findFirstBoundary(std::string &body, size_t *partStart, std::string &fullBoundary)
{
	if (*partStart == std::string::npos)
		return false;
	*partStart += fullBoundary.length();
	if (*partStart < body.length() && body.substr(*partStart, 2) == "\r\n")
		*partStart += 2;
	return true;
}

/**
	* First extracting headers of body-message and call parseHeaders().
	* Adjusting contentStart and extracting content form the whole body-message.
	* RETURN false, if (headerEnd || nextBoundary) == npos.
**/
bool	MultipartParser::extractHeadersAndContent(
							s_extractedData &data,
							std::string &body,
							size_t partStart,
							std::string &fullBoundary,
							size_t *nextBoundary)
{
	std::string	rawHeaders;
	size_t		headersEnd;
	size_t		contentStart;

	headersEnd = body.find("\r\n\r\n", partStart); // Find headers end
	if (headersEnd == std::string::npos)
		return false;

	rawHeaders = body.substr(partStart, headersEnd - partStart);
	data.headers = parseHeaders(rawHeaders);

	contentStart = headersEnd + 4;
	*nextBoundary = body.find("\r\n" + fullBoundary, contentStart);
	if (*nextBoundary == std::string::npos)
		return false;
	data.content = body.substr(contentStart, *nextBoundary - contentStart);
	return true;
}

/**
	* creates a new FormField
**/
bool	MultipartParser::createFormField(t_extractedData &data)
{
	std::map<std::string, std::string> disposition;
	t_FormField	field;
	std::string	fieldName;

	disposition = data.headers["Content-Disposition"];
	if (disposition.count("name") > 0)
	{
		fieldName = disposition["name"];
		field.value = data.content;
		field.filename = disposition.count("filename") > 0 ? disposition["filename"] : "";
		field.isFile = !field.filename.empty();
		if (data.headers.count("Content-Type") > 0)
			field.contentType = data.headers["Content-Type"]["__value"];
		else
			field.contentType = field.isFile ? "application/octet-stream" : "text/plain";
		_result[fieldName] = field;

		std::cout << "\nParsed field: " << fieldName << std::endl;
		std::cout << "  isFile: " << (field.isFile ? "yes" : "no") << std::endl;
		std::cout << "  filename: " << field.filename << std::endl;
		std::cout << "  contentType: " << field.contentType << std::endl;
		std::cout << "  value length: " << field.value.length() << std::endl;
	}
	return true;
}

void	MultipartParser::printParsedResult()
{
	std::cout << "\n=== Parsed Multipart Result ===" << std::endl;
	std::cout << "Total fields: " << _result.size() << std::endl;

	std::map<std::string, t_FormField>::iterator it = _result.begin();
	std::map<std::string, t_FormField>::iterator ite = _result.end();

	while (it != ite)
	{
		std::cout << "\nField: \"" << it->first << "\"" << std::endl;
		std::cout << "  Type: " << (it->second.isFile ? "FILE" : "TEXT") << std::endl;
		std::cout << "  Content-Type: " << it->second.contentType << std::endl;
		if (it->second.isFile)
		{
			std::cout << "  Filename: " << it->second.filename << std::endl;
			std::cout << "  Size: " << it->second.value.length() << " bytes" << std::endl;
			// std::cout << "  Value: \"" << it->second.value << "\"" << std::endl;
		}
		else
		{
			std::cout << "  Value: \"" << it->second.value << "\"" << std::endl;
		}
		++it;
	}
	std::cout << "==============================\n" << std::endl;
}