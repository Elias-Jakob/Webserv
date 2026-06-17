#ifndef AMethod_HPP
# define AMethod_HPP
# include <iostream>
# include <map>
# include <vector>
# include "../../structs.h"
# include "../http/HttpStatus.hpp"
# include "../http/HttpRequest.hpp"

class AMethod
{
	private:
		AMethod(const AMethod& other);
		AMethod &operator=(const AMethod &other);

	protected:
		std::string _method;
		std::string _resource;
		std::string	_body;

		std::string	_phrase;
		std::string	_code;
		std::map<std::string,
				std::vector<std::string> >	_headers;

		std::string	_contentType;
		s_ContentData	_contentData;
		std::map<std::string, s_FormField>	_parsedBody;

	public:
		AMethod();
		AMethod(std::string name);
		virtual ~AMethod();

		bool	setRequiredData(
						s_RequestLine &reqLine,
                        std::map<std::string,
                        std::vector<std::string> > &reqHeads,
                        std::map<std::string, s_FormField> &parsedResult,
                        s_ContentData &contentData);

		void	setResource(std::string &reqURI, std::string &host);
		void	setHeaders(std::map<std::string, std::vector<std::string> > &heads);
		void	setBody(std::map<std::string, s_FormField> &parsedBody);
		void	setContentData(s_ContentData contentData);
		
		virtual bool execute() = 0;

		std::string &getBody();

		// status infos
		std::string &getPhrase();
		std::string	&getCode();
		std::string getContentType();
};

#endif