#ifndef CLIENT_CONNECTION_HPP
# define CLIENT_CONNECTION_HPP

# include "HttpRequest.hpp"
# include "MethodExecuter.hpp"
# include "ResponseBuilder.hpp"
# include "SessionManager.hpp"

# include <ctime>
# include "structs.h"
# include <sys/wait.h>
# include <signal.h>

enum ConnectionState
{
	IDLE,
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
		~ClientConnection();

		int				fd;
		ConnectionState	state;
		std::string		cgi_path;
		std::string		request_buffer;
		std::string		response_buffer;
		size_t			bytesSent;
		HttpRequest		*request;
		AMethod			*_currentMethod;
		MethodExecuter	*executor;
		ResponseBuilder	*responseBuilder;
		SessionManager	*sessionManager;
		bool			keep_alive;
		time_t			inactiveTime;
		
		bool			sendCookie; // for session-management
		std::string		cookieHeader; // for session-management
		std::string		sessionCookie;

		std::time_t	cgiStartTime;
		pid_t	cgiPid;
		int	cgiIn;
		int	cgiOut;
		size_t	cgiWrittenBytes;
		std::string		_listeningInterface;
		std::string		remoteAddr;
		bool	timeout;

		void	processRequest();
		void	cleanUpClient();
		void	applyCgiSessionHeaders();
	private:
		void	executeRequest();
		void	deleteMethod();
		void	sessionHandling();
};

#endif
