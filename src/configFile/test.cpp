# include "ConfigFileParser.hpp"

int main(int argc, char *argv[])
{
	ConfigFileParser parser;
	t_Server	serverData;
	if (argc == 2)
		parser.parseFile(argv[1]);

	serverData = parser.getServerConfigData();
	std::cout << "Server: " << serverData.serverName << std::endl;
	return 0;
}