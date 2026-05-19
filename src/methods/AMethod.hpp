#ifndef AMethod_HPP
# define AMethod_HPP
# include <iostream>
# include <map>
# include <vector>

class AMethod
{
	private:
		AMethod(const AMethod& other);
		AMethod &operator=(const AMethod &other);

	protected:
		std::string _AMethod;
		std::string _resource;
		std::string	_body;

		std::string	_phrase;
		std::string	_code;
		std::map<std::string,
				std::vector<std::string> >	_headers;

		std::string	_contentType;

	public:
		AMethod();
		AMethod(std::string name);
		virtual ~AMethod();

		void setResource(std::string &reqURI, std::string &host);
		void setHeaders(std::map<std::string, std::vector<std::string> > &heads);
		void setBody(std::string &body);

		virtual bool execute(void) = 0;
		std::string &getBody();

		// status infos
		std::string &getPhrase();
		std::string	&getCode();
		std::string getContentType();
};

#endif