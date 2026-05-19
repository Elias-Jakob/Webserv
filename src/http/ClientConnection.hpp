#ifndef CLIENT_CONNECTION_HPP
# define CLIENT_CONNECTION_HPP
# include "HttpRequest.hpp"
# include "HttpResponse.hpp"

enum ConnectionState
{
	READING_REQUEST,
	PROCESSING,
	SENDING_RESPONSE
};

class ClientConnection
{
	public:
		int				fd;
		ConnectionState	state;
		std::string		request_buffer;
		std::string		response_buffer;
		size_t			bytes_sent;
		HttpRequest*	request;
		HttpResponse*	response;

		ClientConnection();
		~ClientConnection();
};

#endif