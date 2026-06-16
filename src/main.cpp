# include "Server.hpp"

int	main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	// TODO: config file parsing
	Server	serv("127.0.0.1", "8080"); // server constructor will take the config class as argument
	
	try {
		serv.serverStartup();
	}
	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
