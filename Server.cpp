#include "Server.hpp"

Server::Server(int port, std::string const &password) : _port(port), _password(password), _parser(new Parser()) {}

Server::~Server() 
{
	delete _parser;
	close(_server_fd);
}

void	Server::setUpSocket()
{
	_server_fd = socket(AF_INET, SOCK_STREAM, 0); // Creates TCP socket
	if (_server_fd < 0)
		throw std::runtime_error("Error: Failed to create socket.");
	
	fcntl(_server_fd, F_SETFL, O_NONBLOCK); // Making it non-blocking -> poll() can handle many clients

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = INADDR_ANY; // Modify socket to accept any incoming IP on specified port

	// Next bind socket, begin listening and add server socket to poll list

	if (bind(_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("Error: Bind failed.");
	
	if (listen(_server_fd, SOMAXCONN) <  0)
		throw std::runtime_error("Error: Listen failed.");

	struct pollfd server_pollfd = {_server_fd, POLLIN, 0};
	_poll_fds.push_back(server_pollfd);

	std::cout << "Server started on port " << _port << std::endl;
}

void	Server::acceptNewClient()
{
	sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(_server_fd, (sockaddr*)&client_addr, &client_len); // Accept a client connection
	if (client_fd < 0)
		return ;

	fcntl(client_fd, F_SETFL, O_NONBLOCK); // Set socket to non blocking
	_poll_fds.push_back({client_fd, POLLIN, 0}); // Add to poll
	
	std::string tempNick = "Guest" + std::to_string(client_fd); // Assign temporary nickname
	_clients[client_fd] = std::make_shared<User>(tempNick, client_fd);

	std::cout << "New client connected: FD " << client_fd << std::endl;
}

void	Server::handleClientInput(int client_fd)
{
	char buffer[512];
	ssize_t bytesRead = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		removeClient(client_fd);
		return;
	}

	buffer[bytesRead] = '\0';
	std::string input(buffer);
	std::cout << "DEBUG!! handleClientInput called for FD: " << client_fd << std::endl;
	std::cout << "DEBUG!! bytesRead = " << bytesRead << std::endl;
	std::cout << "DEBUG!! input = " << input << std::endl;

	auto parsed = _parser->parse(input);
	if (!parsed)
	{
		std::cout << "DEBUG!! Parsing failed for input: " << input << std::endl;
		_clients[client_fd]->sendMessage("Error: Invalid command.");
		return;
	}
	dispatchCommand(_clients[client_fd], *parsed);
}

void	Server::dispatchCommand(std::shared_ptr<User> client, ParsedInput const &parsed)
{
	if (parsed.command == "JOIN" && !parsed.parameters.empty())
	{
		const std::string &channelName = parsed.parameters[0];

		if (_channels.count(channelName) == 0)
			_channels.emplace(std::piecewise_construct, std::forward_as_tuple(channelName), std::forward_as_tuple(channelName));

		std::shared_ptr<User> userPtr = client;
		auto it = _channels.find(channelName);
		if (it != _channels.end() && it->second.addUser(userPtr))
			client->sendMessage("Joined channel " + channelName);
		else
			client->sendMessage("Error: Failed to join channel " + channelName);
	}
	else
	{
		client->sendMessage("Error: Command not handled: " + parsed.command);
	}
}

void	Server::removeClient(int client_fd)
{
	std::vector<struct pollfd>::iterator it = _poll_fds.begin();
	while (it != _poll_fds.end())
	{
		if (it->fd == client_fd)
		{
			_poll_fds.erase(it);
			break;
		}
		it++;
	}

	if (_clients.count(client_fd))
	{
		std::string nickname = _clients[client_fd]->getNickname();
		std::cout << "Client disconnected: " << nickname << " FD " << client_fd << std::endl;
		_clients.erase(client_fd);
	}
	close(client_fd);
}

void	Server::run() // Main server loop
{
	setUpSocket();

	while (true)
	{
		int ret = poll(_poll_fds.data(), _poll_fds.size(), -1); // Wait activity on any socket
		if (ret < 0)
			throw std::runtime_error("Error: Poll failed.");

		size_t i = 0;
		while (i < _poll_fds.size())
		{
			if (_poll_fds[i].revents & POLLIN)
			{
				if (_poll_fds[i].fd == _server_fd) // If new connection call acceptNewClient
					acceptNewClient();
				else
					handleClientInput(_poll_fds[i].fd); // If an existing client call handleInput
			}
			i++;
		}
	}
}

