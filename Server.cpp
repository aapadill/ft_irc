#include "Server.hpp"

Server::Server(int port, std::string const &password) : _port(port), _password(password), _parser(nullptr)
{
	if (port < 0 || port > 65535)
		throw std::invalid_argument("Invalid port number.");

	_parser = new Parser(); //once port is valid, to avoid leaks
}

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

	// add received data to user's buffer
	_clients[client_fd]->appendToBuffer(input);
	
	// process all complete messages in the buffer
	while (_clients.count(client_fd) && _clients[client_fd]->hasCompleteMessage())
	{
		std::string completeMessage = _clients[client_fd]->extractFromBuffer();
		std::cout << "DEBUG!! Processing complete message: " << completeMessage << std::endl;
		
		auto parsed = _parser->parse(completeMessage);
		if (!parsed)
		{
			std::cout << "DEBUG!! Parsing failed for message: " << completeMessage << std::endl;
			if (_clients.count(client_fd))
				_clients[client_fd]->sendMessage("Error: Invalid command.");
			continue;
		}
		if (_clients.count(client_fd))
			dispatchCommand(_clients[client_fd], *parsed);
	}
}

void	Server::dispatchCommand(std::shared_ptr<User> client, ParsedInput const &parsed)
{
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
	else if (command == "JOIN")
		handleJOIN(client, params);
	else if (command == "PART")
		handlePART(client, params);
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

	Channel* channel = getChannelIfExists(channelName, client);
	if (!channel)
		return;
	
	if (!requireChannelOperator(*channel, client, channelName))
		return;
	
	// notify channel about kick
	std::string kickMessage = ":" + client->getNickname() + " KICK " + channelName + " " + targetNick + " :" + reason;
	channel->broadcast(kickMessage);
	
	// remove user from channel
	channel->removeUser(targetNick);
	
	// remove channel if empty
	if (channel->getUsers().empty())
	{
		_channels.erase(channelName);
		std::cout << "DEBUG!! Removed empty channel: " << channelName << std::endl;
	}
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
	
	Channel* channel = getChannelIfExists(channelName, client);
	if (!channel)
		return;
	
	if (!requireChannelOperator(*channel, client, channelName))
		return;
	
	std::shared_ptr<User> targetUser = findUserByNick(targetNick);
	if (!targetUser)
	{
		client->sendNumericReply(401, targetNick + " :No such nick/channel");
		return;
	}
	
	// send invite
	channel->inviteUser(client->getNickname(), targetNick);
	
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
		
		// broadcast topic change to all users including the setter
		std::string topicMessage = ":" + client->getNickname() + " TOPIC " + channelName + " :" + newTopic;
		channel.broadcast(topicMessage); // empty excludeNick means send to all
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
			
			// broadcast mode change to all users including the setter
			channel.broadcast(modeMessage); // empty excludeNick means send to all
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
	// segfault fixed when empty line 
	size_t trim_pos = received.find_last_not_of("\r\n");
	if (trim_pos != std::string::npos)
		received.erase(trim_pos + 1);
	else
		received.clear();

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
	if (!requireRegistration(client, "PRIVMSG"))
		return;

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
		std::cout << "DEBUG !! PRIVMSG from [" << client->getNickname() << "] to [" << receiver << "]" << std::endl;

		std::shared_ptr<User> target = findUserByNick(receiver);
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


void	Server::handleJOIN(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (!requireRegistration(client, "JOIN"))
		return;

	if (params.empty())
	{
		client->sendNumericReply(461, "JOIN :Not enough parameters");
		return;
	}

	const std::string& channelName = params[0];
	
	// validate channel name
	if (channelName.empty() || channelName[0] != '#')
	{
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}

	// create channel if it doesn't exist
	bool isNewChannel = (_channels.count(channelName) == 0);
	if (isNewChannel)
	{
		_channels.emplace(std::piecewise_construct, 
						  std::forward_as_tuple(channelName), 
						  std::forward_as_tuple(channelName));
	}

	Channel& channel = _channels.at(channelName);
	
	// try to add user to channel
	std::string key = "";
	if (params.size() > 1)
		key = params[1];
		
	if (channel.addUser(client, key))
	{
		// If new channel, make the first user an operator
		if (isNewChannel)
		{
			channel.addOperator(client->getNickname());
		}
		
		// send JOIN message to all users in channel including the joiner
		std::string joinMsg = ":" + client->getNickname() + " JOIN " + channelName;
		// send JOIN message to all users in channel including the joiner
		channel.broadcast(joinMsg);
		
		// send topic if exists
		std::string topic = channel.getTopic();
		if (!topic.empty())
		{
			client->sendNumericReply(332, channelName + " :" + topic);
		}
		else
		{
			client->sendNumericReply(331, channelName + " :No topic is set");
		}
		
		// send channel names list (353 and 366)
		std::string namesList = "";
		auto users = channel.getUsers();
		for (const auto& [nick, user] : users)
		{
			if (!namesList.empty())
				namesList += " ";
			if (channel.isOperator(nick))
				namesList += "@";
			namesList += nick;
		}
		client->sendNumericReply(353, "= " + channelName + " :" + namesList);
		client->sendNumericReply(366, channelName + " :End of /NAMES list");
	}
	else
	{
		client->sendNumericReply(473, channelName + " :Cannot join channel");
	}
}

void	Server::handlePART(std::shared_ptr<User> client, const std::vector<std::string>& params)
{
	if (!requireRegistration(client, "PART"))
		return;

	if (params.empty())
	{
		client->sendNumericReply(461, "PART :Not enough parameters");
		return;
	}

	const std::string& channelName = params[0];
	
	Channel* channel = getChannelIfExists(channelName, client);
	if (!channel)
		return;
	
	if (!channel->hasUser(client->getNickname()))
	{
		client->sendNumericReply(442, channelName + " :You're not on that channel");
		return;
	}

	// part message
	std::string partMsg = "Leaving";
	if (params.size() > 1)
		partMsg = params[1];

	// send PART message to all users in channel including the one leaving
	std::string fullMsg = ":" + client->getNickname() + " PART " + channelName + " :" + partMsg;
	channel->broadcast(fullMsg);
	
	// remove user from channel
	channel->removeUser(client->getNickname());
	
	// remove channel if empty
	if (channel->getUsers().empty())
	{
		_channels.erase(channelName);
		std::cout << "DEBUG!! Removed empty channel: " << channelName << std::endl;
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
	std::string clientNick = client->getNickname(); // Store nickname before removal
	
	// send quit message to client first
	std::cout << "DEBUG!! QUIT: " << fullMsg << std::endl;
	client->sendMessage(fullMsg);

	// inform all users in channels and clean up empty channels
	std::vector<std::string> emptyChannels;
	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		Channel& channel = it->second;
		if (channel.hasUser(clientNick))
		{
			channel.broadcast(fullMsg, clientNick);
			channel.removeUser(clientNick);
			
			// Mark channel for removal if empty
			if (channel.getUsers().empty())
				emptyChannels.push_back(it->first);
		}
	}
	
	// remove empty channels
	for (const std::string& channelName : emptyChannels)
	{
		_channels.erase(channelName);
		std::cout << "DEBUG!! Removed empty channel: " << channelName << std::endl;
	}
	
	// remove the client at end of connection
	removeClient(client->getSocket());
}

// helpers
std::shared_ptr<User> Server::findUserByNick(const std::string& nickname)
{
	for (const auto& [fd, user] : _clients)
	{
		if (user->getNickname() == nickname)
			return user;
	}
	return nullptr;
}

bool Server::requireRegistration(std::shared_ptr<User> client, const std::string& command)
{
	if (!client->isRegistered())
	{
		client->sendNumericReply(451, command + " :You have not registered");
		return false;
	}
	return true;
}

Channel* Server::getChannelIfExists(const std::string& channelName, std::shared_ptr<User> client)
{
	if (_channels.count(channelName) == 0)
	{
		client->sendNumericReply(403, channelName + " :No such channel");
		return nullptr;
	}
	return &_channels.at(channelName);
}

bool Server::requireChannelOperator(Channel& channel, std::shared_ptr<User> client, const std::string& channelName)
{
	if (!channel.isOperator(client->getNickname()))
	{
		client->sendNumericReply(482, channelName + " :You're not channel operator");
		return false;
	}
	return true;
}

