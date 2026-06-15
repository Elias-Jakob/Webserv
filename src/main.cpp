# include "Server.hpp"

int	main(int argc, int *argv[])
{
	// TODO: config file parsing
	char	*interface = "127.0.0.1", *port = "8080"; // this should be a std::map in the config class (interface:port pairs)
	Server	serv("127.0.0.1", "8080"); // server constructor will take the config class as argument
	
	try {
		serv.serverStartup();
	}
	catch (const std::exception &e) {
		std::err << "Error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
