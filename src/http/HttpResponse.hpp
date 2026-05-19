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

	// needed datas for response
		AMethod 			*method; // Abstract class for AMethods (GET, POST, DELETE)
		std::string		statusLine;
		t_status		status;

		t_responseHeaders	_resHeads; // Headers to sent back (Date: , Content-Length, Content-Type, ..)

		std::string		messageHeaders; // store as map[key] = value ?
		std::string		messageBody;


	// private AMethods
		static AMethod	*createGet(std::string name);
		static AMethod	*createPost(std::string name);

		bool			buildStatusLine();
		void			setContentLength();
		void			buildHeaders();

	public:
		// construction & deconstruction
		HttpResponse();
		~HttpResponse();
		HttpResponse(HttpRequest *request);

		void buildResponse(HttpRequest *request);

		// AMethods
		std::string &getStatusLine();
		std::string &getMessageBody();
		std::string &getMessageHeaders();
};

#endif