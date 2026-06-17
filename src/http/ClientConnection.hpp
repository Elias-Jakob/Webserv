#ifndef CLIENT_CONNECTION_HPP
# define CLIENT_CONNECTION_HPP
# include "HttpRequest.hpp"
# include "MethodExecuter.hpp"
# include "ResponseBuilder.hpp"
# include "../../structs.h"

enum ConnectionState
{
	READING_REQUEST,
	PROCESSING,
	SENDING_RESPONSE
};

/**
	*@class ClientConnection
	*@brief Class which handles the Request from the Client.
	* PARSING the Request-message, 
	* EXECUTING the requested Method, 
	* BUILDING the response message.
*/
class ClientConnection
{
	public:
		int				fd;
		ConnectionState	state;
		std::string		request_buffer;
		std::string		response_buffer;
		size_t			bytes_sent;
		HttpRequest*	request;
		AMethod			*_currentMethod;
		MethodExecuter*	executor;
		ResponseBuilder	*responseBuilder;
		bool			keep_alive;

		ClientConnection();
		~ClientConnection();

		void	processRequest();
		void	cleanUpClient();
};

#endif