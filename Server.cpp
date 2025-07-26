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
	/*
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
	*/
	const std::string &command = parsed.command;
	const std::vector<std::string> &params = parsed.parameters;
	if (command == "PASS")
	{
		handlePASS(client, params);
		return;
	}
	else if (command == "KICK")
		handleKICK(client, params);
	else if (command == "INVITE")
		handleINVITE(client, params);
	else if (command == "TOPIC")
		handleTOPIC(client, params);
	else if (command == "MODE")
		handleMODE(client, params);
	else if (command == "NICK")
		handleNICK(client, params);
	else if (command == "USER")
		handleUSER(client, params);
	else if (command == "PRIVMSG")
		handlePRIVMSG(client, params);
	else if (command == "NOTICE")
		handleNOTICE(client, params);
	else if (command == "QUIT")
		handleQUIT(client, params);
	else
	{
		client->sendNumericReply(421, parsed.command + " :Unknown command");
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

void Server::handleKICK(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (params.size() < 2)
	{
		client->sendNumericReply(461, "KICK :Not enough parameters");
		return;
	}
	
	const std::string& channelName = params[0];
	const std::string& targetNick = params[1];
	std::string reason;

	if (params.size() > 2)
	{
		reason = params[2];
	}
	else
	{
		reason = client->getNickname();
	}


	if (_channels.count(channelName) == 0)
	{
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}
	
	Channel& channel = _channels.at(channelName);
	
	// check if user is operator
	if (!channel.isOperator(client->getNickname()))
	{
		client->sendNumericReply(482, channelName + " :You're not channel operator");
		return;
	}
	
	// notify channel about kick
	std::string kickMessage = ":" + client->getNickname() + " KICK " + channelName + " " + targetNick + " :" + reason;
	channel.broadcast(kickMessage);
	
	// remove user from channel
	channel.removeUser(targetNick);
}

void Server::handleINVITE(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (params.size() < 2)
	{
		client->sendNumericReply(461, "INVITE :Not enough parameters");
		return;
	}
	
	const std::string& targetNick = params[0];
	const std::string& channelName = params[1];
	
	if (_channels.count(channelName) == 0)
	{
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}
	
	Channel& channel = _channels.at(channelName);
	
	// check if user is operator
	if (!channel.isOperator(client->getNickname()))
	{
		client->sendNumericReply(482, channelName + " :You're not channel operator");
		return;
	}
	
	// find target user
	std::shared_ptr<User> targetUser = nullptr;
	for (const auto& [fd, user] : _clients)
	{
		if (user->getNickname() == targetNick)
		{
			targetUser = user;
			break;
		}
	}
	
	if (!targetUser)
	{
		client->sendNumericReply(401, targetNick + " :No such nick/channel");
		return;
	}
	
	// send invite
	channel.inviteUser(client->getNickname(), targetNick);
	
	// notify both users
	client->sendNumericReply(341, targetNick + " " + channelName);
	targetUser->sendMessage(":" + client->getNickname() + " INVITE " + targetNick + " " + channelName);
}

void Server::handleTOPIC(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (params.empty())
	{
		client->sendNumericReply(461, "TOPIC :Not enough parameters");
		return;
	}
	
	const std::string& channelName = params[0];
	
	if (_channels.count(channelName) == 0)
	{
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}
	
	Channel& channel = _channels.at(channelName);
	
	if (params.size() == 1)
	{
		// get topic
		std::string topic = channel.getTopic();
		if (topic.empty())
			client->sendNumericReply(331, channelName + " :No topic is set");
		else
			client->sendNumericReply(332, channelName + " :" + topic);
	}
	else
	{
		// set topic
		const std::string& newTopic = params[1];
		channel.setTopic(client->getNickname(), newTopic);
		
		// broadcast topic change
		std::string topicMessage = ":" + client->getNickname() + " TOPIC " + channelName + " :" + newTopic;
		channel.broadcast(topicMessage);
		client->sendMessage(topicMessage); // also send to the user who set it
	}
}

void Server::handleMODE(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (params.empty())
	{
		client->sendNumericReply(461, "MODE :Not enough parameters");
		return;
	}
	
	const std::string& channelName = params[0];
	
	if (_channels.count(channelName) == 0)
	{
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}
	
	Channel& channel = _channels.at(channelName);
	
	// check if user is operator
	if (!channel.isOperator(client->getNickname()))
	{
		client->sendNumericReply(482, channelName + " :You're not channel operator");
		return;
	}
	
	if (params.size() == 1)
	{
		// get channel modes (simplified)
		client->sendNumericReply(324, channelName + " +nt");
		return;
	}
	
	const std::string& modeString = params[1];
	size_t paramIndex = 2;
	bool adding = true;
	
	for (char c : modeString)
	{
		if (c == '+')
		{
			adding = true;
		}
		else if (c == '-')
		{
			adding = false;
		}
		else
		{
			std::string arg = "";
			if ((c == 'k' || c == 'l' || c == 'o') && paramIndex < params.size())
			{
				arg = params[paramIndex++];
			}
			
			channel.setMode(c, adding, arg);
			
			// broadcast change of mode
			std::string modeMessage = ":" + client->getNickname() + " MODE " + channelName + " " + 
									(adding ? "+" : "-") + c;
			if (!arg.empty())
				modeMessage += " " + arg;
			
			channel.broadcast(modeMessage);
			client->sendMessage(modeMessage);
		}
	}
}

void	Server::handleNICK(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (!client->isAuthenticated())
	{
		client->sendNumericReply(451, "NICK :You have not registered (missing PASS)");
		return;
	}
	if (params.empty())
	{
		client->sendNumericReply(431, "NICK :No nickname given");
		return;
	}

	if (client->isRegistered())
	{
		client->sendNumericReply(462, "NICK :You may not re-register");
		return;
	}

	std::string newNick = params[0];
	size_t end = newNick.find_last_not_of("\r\n");
	if (end != std::string::npos)
		newNick.erase(end + 1);
	
	if (!client->isValidNickname(newNick))
	{
		client->sendNumericReply(432, newNick + ":Invalid nickname");
		return;
	}

	for (const auto& [fd, user] : _clients)
	{
		if (user->getNickname() == newNick)
		{
			client->sendNumericReply(433, newNick + " :Nickname is already in use");
			return;
		}
	}

	client->setNickname(newNick);
	std::cout << "DEBUG!! Set nickname to " << newNick << " for FD: " << client->getSocket() << std::endl;

	client->checkRegisteration();
}

void	Server::handleUSER(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (!client->isAuthenticated())
	{
		client->sendNumericReply(451, "USER :You have not registered (missing PASS)");
		return;
	}
	if (params.size() < 4 || params[3].empty() /*|| params[3][0] != ':'*/)
	{
		client->sendNumericReply(461, "USER :Not enough parameters");
		return;
	}
	if (client->isRegistered())
	{
		client->sendNumericReply(462, "USER :You may not re-register");
		return;
	}

	std::string username = params[0];
	std::string realname = params[3];

	if (!username.empty() && (username.back() == '\r' || username.back() == '\n'))
		username.erase(username.find_last_not_of("\r\n") + 1);
	if (!realname.empty() && (realname.back() == '\r' || realname.back() == '\n'))
		realname.erase(realname.find_last_not_of("\r\n") + 1);

	if (!client->isValidUsername(username))
	{
		client->sendNumericReply(468, ":Invalid username");
		return;
	}

	if (!client->isValidRealname(realname))
	{
		client->sendNumericReply(468, ":Invalid realname");
		return;
	}

	client->setUsername(username);
	client->setRealname(realname);

	client->setAuthenticated(true);
	std::cout << "DEBUG!! Set username to " << params[0] << ", realname to " << params[3] << std::endl;

	client->checkRegisteration();
}

void	Server::handlePASS(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (params.empty())
	{
		client->sendNumericReply(461, " PASS: Not enough parameters");
		return ;
	}
	std::cout << "DEBUG!! Excpected password: " << _password << std::endl;
	std::cout << "DEBUG!! Received password: " << params[0] << std::endl;

	std::string received = params[0];
	received.erase(received.find_last_not_of("\r\n") + 1);

	std::cout << "DEBUG!! Expected password: [" << _password << "]" << std::endl;
	std::cout << "DEBUG!! Received password: [" << received << "]" << std::endl;

	if (client->isAuthenticated())
	{
		client->sendNumericReply(462, ":You may not re-register");
		return;
	}

	/*if (params.empty())
	{
		client->sendNumericReply(461, "PASS :Not enough parameters");
		return;
	}*/

	if (received != _password)
	{
		client->sendNumericReply(464, ":Password incorrect");
		return;
	}

	client->setAuthenticated(true);
}

void	Server::handlePRIVMSG(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (!client->isRegistered())
	{
		client->sendNumericReply(451, "PRIVMSG :You have not registered");
		return;
	}

	if (params.size() < 2)
	{
		client->sendNumericReply(461, "PRIVMSG :Not enough parameters");
		return;
	}

	const std::string& receiver = params[0];
	const std::string& message = params[1];

	if (receiver[0] == '#')
	{
		if (_channels.count(receiver) == 0)
		{
			client->sendNumericReply(403, receiver + " :No such channel");
			return;
		}

		Channel& channel = _channels.at(receiver);
		if (!channel.hasUser(client->getNickname()))
		{
			client->sendNumericReply(404, receiver + " :Cannot send to channel");
			return;
		}

		std::string fullMsg = ":" + client->getNickname() + " PRIVMSG " + receiver + " :" + message;
		channel.broadcast(fullMsg, client->getNickname());
	}
	else //privmsg to another user
	{
		std::shared_ptr<User> target = nullptr;
		std::cout << "DEBUG !! PRIVMSG from [" << client->getNickname() << "] to [" << receiver << "]" << std::endl;

		for (const auto& [fd, user] : _clients)
		{
			std::cout << "DEBUG!! Checking against registered user: [" << user->getNickname() << "]" << std::endl;
			if (user->getNickname() == receiver)
			{
				target = user;
				break;
			}
		}

		if (!target)
		{
			client->sendNumericReply(401, receiver + " :No such nick");
			return;
		}

		std::string fullMsg = ":" + client->getNickname() + " PRIVMSG " + receiver + " :" + message;
		std::cout << "DEBUG!! Sending message to " << target->getNickname() << ": " << fullMsg << std::endl;
		target->sendMessage(fullMsg);
	}
}

void	Server::handleNOTICE(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (!client->isRegistered())
		return;
	
	if (params.size() < 2)
		return;

	const std::string& receiver = params[0];
	const std::string& message = params[1];

	if (receiver[0] == '#')
	{
		if (_channels.count(receiver) == 0)
			return;

		Channel& channel = _channels.at(receiver);
		if (!channel.hasUser(client->getNickname()))
			return;

		std::string fullMsg = ":" + client->getNickname() + " NOTICE " + receiver + " :" + message;
		channel.broadcast(fullMsg, client->getNickname());
	}
	else
	{
		for (const auto& [fd, user] : _clients)
		{
			if (user->getNickname() == receiver)
			{
				std::string fullMsg = ":" + client->getNickname() + " NOTICE " + receiver + " :" + message;
				user->sendMessage(fullMsg);
				break;
			}
		}
	}
}


void	Server::handleQUIT(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	//not fully tested
	std::string quitMsg = "Client Quit";
	if (!params.empty())
	{
		quitMsg = params[0];
		if (quitMsg[0] == ':')
			quitMsg = quitMsg.substr(1);
	}

	std::string fullMsg = ":" + client->getNickname() + " QUIT :" + quitMsg;

	//informing all users in channels about quitting
	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		Channel& channel = it->second;
		if (channel.hasUser(client->getNickname()))
			channel.broadcast(fullMsg, client->getNickname());
		channel.removeUser(client->getNickname());
	}

	std::cout << "DEBUG!! QUIT: " << fullMsg << std::endl;
	client->sendMessage(fullMsg);
	close(client->getSocket());
	_clients.erase(client->getSocket());
}

