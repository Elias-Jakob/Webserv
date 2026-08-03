# include "Server.hpp"

sig_atomic_t	sigFlag = 0;

void	sigIntHandler(int sig)
{
	sigFlag = sig;
}

int	main(int argc, char *argv[])
{
	ConfigFileParser	conf;

	try {
		if (argc != 2)
			throw std::invalid_argument("Error: wrong number of arguments...\n"
				"Expected usage: ./webserv <webserver.conf>");
		// TODO: signal
		if (signal(SIGINT, &sigIntHandler) == SIG_ERR || signal(SIGPIPE, SIG_IGN) == SIG_ERR) // TODO: handle SIGPIPE as well?
			throw std::runtime_error(std::strerror(errno));
		//
		Server	serv(conf.parseFile(argv[1]));
		serv.serverStartup();
	}
	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	catch (...) {
		return (1);
	}
	return (0);
}
