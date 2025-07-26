#include "Server.hpp"

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>\n";
		return 1;
	}

	int port = 0;
	try
	{
		port = std::stoi(argv[1]);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: Invalid port number\n";
		return 1;
	}

	std::string password = argv[2];

	try 
	{
		Server server(port, password);
		server.run();
	}
	catch (const std::exception& e) 
	{
		std::cerr << "Fatal Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
