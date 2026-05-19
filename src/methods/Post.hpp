#ifndef POST_HPP
# define POST_HPP

# include "AMethod.hpp"
# include <fstream>
# include <sys/stat.h>
# include "../parsers/FormParser.hpp"
# include "../parsers/MultipartParser.hpp"

typedef struct s_ContentData
{
	std::string type;
	std::string subtype;
	std::string boundary;
}				t_ContentData;

class Post : public AMethod
{
	private:
		std::string parse_type;
		std::map<std::string, std::string>	parsed_body;
		s_ContentData	_contentData;

	public:
		Post();
		Post(std::string name);
		~Post();

		bool execute();

		void parse_form_data(std::string &string);
		std::string parseContentType(std::vector<std::string> value);
};

#endif