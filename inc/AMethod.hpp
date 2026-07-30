#ifndef AMethod_HPP
# define AMethod_HPP

# include <iostream>
# include <map>
# include <vector>
# include <sys/stat.h>

# include "structs.h"
# include "HttpStatus.hpp"
# include "HttpRequest.hpp"
# include "ConfigFileParser.hpp"

/** 
	* @class AMethod
	* @brief Concrete-Class to build the Methods on.
*/
class AMethod
{
	public:
		AMethod();
		AMethod(std::string name);
		AMethod(std::string name, t_Location *location);

		virtual ~AMethod();
		virtual bool execute() = 0;

		bool	isDirList();
		bool	isUploadLocation();
		bool	isSubmitLocation();

		bool    setRequiredData(HttpRequest *req, const std::string modifiedURI);
		void	setResource(const std::string &modifiedURI);
		void	setHeaders(std::map<std::string, std::vector<std::string> > &heads);
		void	setBody(std::map<std::string, s_FormField> &parsedBody);
		void	setContentData(s_ContentData contentData);
		void	setReqUri(const std::string &requestURI);
		bool	checkCGI();
		bool	executeCGI(const std::string &script);


		std::string &getBody();
		std::string &getPhrase();
		std::string	&getCode();
		std::string getContentType();
		std::string getRedirectURL();
		std::string	getLastModified();
		std::string	getEtag();

	protected:
		std::string _method;
		std::string	_reqUri;
		std::string	_body;
		std::map<std::string,
				std::vector<std::string> >	_headers;

		std::string _resource;
		std::string	_phrase;
		std::string	_code;

		std::string	_contentType;
		s_ContentData	_contentData;
		std::map<std::string, s_FormField>	_parsedBody;
		t_Location	*_location;
		bool		_isAutoIndex;
		std::string	_lastModified;
		std::string	_etag;

	private:
		AMethod(const AMethod& other);
		AMethod &operator=(const AMethod &other);
};

#endif // AMETHOD_HPP
