#ifndef HTTP_RESPONSE_HPP
# define HTTP_RESPONSE_HPP

# include <iostream>
# include <string>
# include <map>
# include <algorithm>

# include <sstream>
# include <vector>
# include "HttpRequest.hpp"
# include "../methods/AMethod.hpp"
# include "../methods/Get.hpp"
# include "../methods/Post.hpp"
# include "../parsers/ABodyParser.hpp"
# include "../parsers/FormParser.hpp"
# include "../parsers/MultipartParser.hpp"
# include "../../structs.h"

typedef struct	s_status
{
	std::string	httpVersion;
	std::string statusCode;
	std::string	reasonPhrase;
}				t_status;

typedef struct	s_responseHeaders
{
	std::string	contentType;
	std::string	contentLength;
	std::string	Date;
}				t_responseHeaders;

class HttpResponse
{
	private:
		// copies of HttpRequest values.
		s_RequestLine					_reqLine;
		std::map<
			std::string,
			std::vector<std::string> >	_reqHeaders;
		std::string						_reqBody;

		// datas for response
		AMethod 		*method; // Abstract class for AMethods (GET, POST, DELETE)
		ABodyParser		*_parser;
		t_status		status;

		t_responseHeaders	_resHeads; // Headers to sent back (Date: , Content-Length, Content-Type, ..)
		
		std::string		statusLine;
		std::string		messageHeaders; // store as map[key] = value ?
		std::string		messageBody;

		t_ContentData	_contentData;
		std::map<std::string, s_FormField> _parsedResult;

		// create AMethod
		static AMethod	*createGet(std::string name);
		static AMethod	*createPost(std::string name);
		// create ABodyParser
		bool	createBodyParser();
		ABodyParser *createMultiParser();
		ABodyParser *createFormParser();

		bool			buildStatusLine();
		void			setContentLength();
		void			buildHeaders();
		std::string	parseContentType(std::vector<std::string> value);

	public:
		// construction & deconstruction
		HttpResponse();
		~HttpResponse();
		HttpResponse(HttpRequest *request);

		// AMethods
		std::string &getStatusLine();
		std::string &getMessageBody();
		std::string &getMessageHeaders();
};

#endif