#ifndef REQUEST_BODY_PARSER_HPP
# define REQUEST_BODY_PARSER_HPP

# include "structs.h"
# include "limits_defines.hpp"
# include "ABodyParser.hpp"
# include "MultipartParser.hpp"
# include "FormParser.hpp"

class RequestBodyParser
{
	public:
		RequestBodyParser();
		RequestBodyParser(t_RequestData *data_);
		~RequestBodyParser();

		void	parseBody();
		bool	extractBody();

	private:
		t_RequestData	*data;
		ABodyParser		*_bodyParser;

		bool	createBodyParser();
		std::string	parseContentType(std::vector<std::string> value);
		ABodyParser *createMultiParser();
		ABodyParser *createFormParser();
	
		// CHUNKED-TRANSFER
		bool		unchunkBody();
		std::string	chunkedData(size_t chunked_size);
		size_t		chunkedSize();
		
		// HELPER
		bool	validBodySize(size_t contentLength);
		bool	setErrorCode(int code);
};

#endif