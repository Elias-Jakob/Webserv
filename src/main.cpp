# include "Server.hpp"

sig_atomic_t	sigFlag = 0;

void	sigIntHandler(int sig)
{
	sigFlag = sig;
}

int	main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	// TODO: config file parsing
	Server	serv("127.0.0.1", "8080"); // server constructor will take the config class as argument
	
	try {
		// TODO: signal
		if (signal(SIGINT, &sigIntHandler) == SIG_ERR)
			throw std::runtime_error(std::strerror(errno));
		//
		serv.serverStartup();
	}
	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
