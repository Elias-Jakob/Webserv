#ifndef REQUEST_HEADS_PARSER_HPP
# define REQUEST_HEADS_PARSER_HPP

# include "structs.h"
# include "limits_defines.hpp"

class RequestHeadsParser
{
	public:
		RequestHeadsParser();
		RequestHeadsParser(t_RequestData *data_);
		~RequestHeadsParser();

		bool	parseHeaderLine();

	private:
		t_RequestData	*data;
		
		void	addHeader(const std::string &key, const std::string &value);
		std::string	extractHeader(size_t *lineEnd);
		bool	setHeaderPair(const std::string &line);
		bool	validHeaderPair(const std::string &key, const std::string &value);
		std::vector<std::string> splitHeaderValByComma(std::string val);
		std::string	toLowerCase(std::string &str);
		size_t	skipLWS(std::string val, size_t start, size_t end);
		void	adjustCurrentPos(size_t pos);
		void	setCurrentPos(size_t pos);
		bool	setErrorCode(int code);

		// MOVE logic somewhere else?
		std::vector<std::string>	splitPath(const std::string &path);
		void	modifyURI(std::vector<std::string> &pathParts);
		void	modifyURIforCGI();
		void	findLocation(std::vector<std::string> pathParts);
		size_t	posOfScriptName(std::vector<std::string> &parts, std::vector<std::string> cgiExt, size_t n);
		bool	isListeningTo(size_t i, const std::string &listeningInterface);
		void	setScriptName(std::vector<std::string> &parts, size_t n);
		void	setPathInfo(std::vector<std::string> &parts, size_t start);

};

#endif