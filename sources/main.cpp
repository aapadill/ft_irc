#include "Server.hpp"

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>\n" << std::endl;
		return 1;
	}

	int port = std::atoi(argv[1]);
	std::string password = argv[2];

	try 
	{
		Server server(port, password);
		server.run();
	}
	catch (const std::exception& e) 
	{
		std::cerr << RED << "Fatal Error: " << e.what() << DEFAULT << std::endl;
	}

	return 0;
}
