#ifndef REQUEST_LINE_PARSER_HPP
# define REQUEST_LINE_PARSER_HPP

// #include "HttpRequest.hpp"
# include "structs.h"
# include "limits_defines.hpp"

#include <vector>
#include <iostream>
#include <map>

class RequestLineParser
{
	public:
		RequestLineParser();
		RequestLineParser(t_RequestData *data);
		~RequestLineParser();

		bool	parseRequestLine();
	
	private:
		t_RequestData	*data;
		
		void	skipEmptyLines();
		bool	checkForCRandLF();
		bool	setRequestLineParts(const std::string &reqLine, 
									size_t posSP1, 
									size_t posSP2);
		void	handleQuery();
		bool	validMethod();
		bool	validHttpVersion();
		bool	validURI();
		bool	decodeURI();
		void	extractFileExtension();
		void	setQueryPairs(const std::string &query);
		bool	validURIchar(char c);
		bool	validURIstr(std::string &URI);
		bool	setErrorCode(int code);
		bool	foundEndOfRequest();
		void	setQueryKeyValue(const std::string &queryStr, 
								size_t start, 
								size_t posEqual, 
								size_t end);
};

#endif