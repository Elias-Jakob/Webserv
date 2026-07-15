#ifndef CLIENT_CONNECTION_HPP
# define CLIENT_CONNECTION_HPP
# include "HttpRequest.hpp"
# include "MethodExecuter.hpp"
# include "ResponseBuilder.hpp"
# include "structs.h"

enum ConnectionState
{
	READING_REQUEST,
	PROCESSING,
	SENDING_RESPONSE,
	CGI_PROCESSING
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
		ClientConnection();
		// TODO: implement copy constructor and assignment operator
		// ClientConnection(const ClientConnection &other);
		// ClientConnection	&operator=(const ClientConnection &other);
		~ClientConnection();

		int				fd;
		ConnectionState	state;
		std::string		cgi_path;
		std::string		request_buffer;
		std::string		response_buffer;
		size_t			bytesSent;
		HttpRequest*	request;
		AMethod			*_currentMethod;
		MethodExecuter*	executor;
		ResponseBuilder	*responseBuilder;
		bool			keep_alive;
		size_t		inactiveTime;
		std::string		_listeningInterface;

		void	processRequest();
		void	cleanUpClient();

		// CGI
		pid_t	cgiPid;

	private:
		void	executeRequest();
		void	deleteMethod();
		
};

#endif
