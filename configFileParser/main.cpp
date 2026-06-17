# include "configFileParser.hpp"

int main(int argc, char *argv[])
{
	ConfigFileParser parser;

	if (argc == 2)
		parser.parseFile(argv[1]);

	return 0;
}