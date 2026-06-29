#ifndef CGIProcess
# define CGIProcess

# include "ClientConnection.hpp"

const char	*CGI_PATH = "www/cgi-bin/";

class CGIProcess
{
	public:
		CGIProcess();
		CGIProcess(const CGIProcess &other);
		CGIProcess	&operator=(const CGIProcess &other);
		~CGIProcess();
	private:
		ClientConnection	&client;
		int	pid;
		int	inPipe[2];
		int	outPipe[2];
};

#endif // !CGIProcess
