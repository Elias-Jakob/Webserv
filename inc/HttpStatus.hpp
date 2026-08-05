#ifndef HTTP_STATUS_HPP
#define HTTP_STATUS_HPP

#include <string>
#include <map>
#include <sstream>

class HttpStatus
{
	private:
		static std::map<int, std::string> initStatusMap();
		static std::map<int, std::string> statusPhrases;

	public:
		static std::string getPhrase(int code);
		static std::string setStatus(int code, std::string &codeStr, std::string &phraseStr);

	// Common status codes as constants
	static const int OK = 200;
	static const int NO_CONTENT = 204;
	static const int BAD_REQUEST = 400;
	static const int FORBIDDEN = 403;
	static const int NOT_FOUND = 404;
	static const int METHOD_NOT_ALLOWED = 405;
	static const int INTERNAL_SERVER_ERROR = 500;
};

#endif
