# include "HttpStatus.hpp"

std::map<int, std::string> HttpStatus::statusPhrases = HttpStatus::initStatusMap();

std::map<int, std::string> HttpStatus::initStatusMap()
{
	std::map<int, std::string>	phrases;

	// 2xx Success
	phrases[200] = "OK";
	phrases[201] = "Created";
	phrases[204] = "No Content";

	// 3xx Redirection
	phrases[301] = "Moved Permanently";
	phrases[302] = "Found";
	phrases[304] = "Not Modified";

	// 4xx Client Errors
	phrases[400] = "Bad Request";
	phrases[403] = "Forbidden";
	phrases[404] = "Not Found";
	phrases[405] = "Method Not Allowed";
	phrases[411] = "Length Required";
	phrases[413] = "Payload Too Large";
	phrases[414] = "URI Too Long";
	phrases[415] = "Unsupported Media Type";
	phrases[431] = "Bad Request(too many headers)";
	// 5xx Server Errors
	phrases[500] = "Internal Server Error";
	phrases[501] = "Not Implemented";
	phrases[504] = "Gateway Timeout";
	phrases[505] = "HTTP Version Not Supported";

	return phrases;
}

std::string HttpStatus::getPhrase(int code)
{
	if (statusPhrases.find(code) != statusPhrases.end())
		return statusPhrases[code];
	return "Unknown";
}

// Helper: Set both code and phrase at once
std::string HttpStatus::setStatus(int code, std::string &codeStr, std::string &phraseStr)
{
    std::stringstream ss;
    ss << code;
    codeStr = ss.str();
    phraseStr = getPhrase(code);
    return codeStr;
}
