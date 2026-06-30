#ifndef CGIProcess
# define CGIProcess

# include "Server.hpp"

typedef struct	s_CGIProcess
{
	ClientConnection	client;
	int	pid;
} t_CGIProcess;

#endif // !CGIProcess
