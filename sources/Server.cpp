#include "Server.hpp"
#include "Logger.hpp"
#include "Client.hpp"

Server::Server(int port, std::string const &password) : _port(port), _password(password), _parser(new Parser()) {}

Server::~Server() 
{
	delete _parser;
	close(_server_fd);
}

void	Server::setUpSocket()
{
	Logger::log(LogLevel::INFO, "Init Server: Server creation ");
	_server_fd = socket(AF_INET, SOCK_STREAM, 0); // Creates TCP socket
	if (_server_fd < 0)
		throw std::runtime_error("Error: Failed to create socket.");
	
	fcntl(_server_fd, F_SETFL, O_NONBLOCK); // Making it non-blocking -> poll() can handle many clients

	Logger::log(LogLevel::INFO, "Init Server: Binding on port  " + std::to_string(_port));
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

	//std::cout << "Server started on port " << _port << std::endl;
	// if we se logger instead
	Logger::log(LogLevel::INFO, "Server started on port " + std::to_string(_port));
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
	_clients[client_fd] = std::make_shared<Client>(client_fd);

	//std::cout << "New client connected: FD " << client_fd << std::endl;
	Logger::log(LogLevel::INFO, "New client connected: FD " + std::to_string(client_fd));
}

void	Server::handleClientInput(int client_fd)
{
	char buffer[512];
	ssize_t bytesRead = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0) {
		Logger::log(LogLevel::INFO, "Client disconnected or error on FD: " + std::to_string(client_fd));
		removeClient(client_fd);
		return;
	}

	buffer[bytesRead] = '\0';
	std::string input(buffer);
	Logger::log(LogLevel::DEBUG, "handleClientInput called for FD: " + std::to_string(client_fd));
	Logger::log(LogLevel::DEBUG, "bytes read: " + std::to_string(bytesRead));
	Logger::log(LogLevel::DEBUG, "input: " + input);

	std::shared_ptr<Client> client = _clients[client_fd];
	client->appendToBuffer(input);

	while (client->hasCompleteMessage()) {
		std::string message = client->getNextMessage();
		Logger::log(LogLevel::DEBUG, "parsed message: " + message);
		try {
			auto parsed = _parser->parse(input);
			if (!parsed) {
				Logger::log(LogLevel::DEBUG, "Parsing failed for input: " + input);
				client->sendMessage("Error: Invalid command.");
				continue;
			}
			dispatchCommand(client, *parsed);
		} catch (const std::exception& e) {
			Logger::log(LogLevel::WARNING, "Exception during parsing : " + std::string(e.what()));
			client->sendMessage("Error: Command Processing failed");
		}
	}
}

void	Server::dispatchCommand(std::shared_ptr<Client> client, ParsedInput const &parsed)
{
	switch (parsed.commandType) {
		case CommandType::CAP:
			handleCAP(client, parsed);
			break;
		case CommandType::JOIN:
			handleJOIN(client, parsed);
			break;
		//case CommandType::PART:
		//	handlePART(client, parsed);
		//	break;
		case CommandType::KICK:
			handleKICK(client, parsed);
			break;
		case CommandType::INVITE:
			handleINVITE(client, parsed);
			break;
		case CommandType::TOPIC:
			handleTOPIC(client, parsed);
			break;
		case CommandType::MODE:
			handleMODE(client, parsed);
			break;
		case CommandType::NICK:
			handleNICK(client, parsed);
			break;
		case CommandType::USER:
			handleUSER(client, parsed);
			break;
		case CommandType::PASS:
			handlePASS(client, parsed);
			break;
		case CommandType::PRIVMSG:
			handlePRIVMSG(client, parsed);
			break;
		case CommandType::NOTICE:
			handleNOTICE(client, parsed);
			break;
		case CommandType::QUIT:
			handleQUIT(client, parsed);
			break;
		default:
			client->sendNumericReply(421, parsed.command + " :Unknown command");
	}
}

void	Server::removeClient(int client_fd)
{
	std::vector<struct pollfd>::iterator it = _poll_fds.begin();
	while (it != _poll_fds.end()) {
		if (it->fd == client_fd) {
			_poll_fds.erase(it);
			break;
		}
		it++;
	}

	if (_clients.count(client_fd)) {
		std::string nickname = _clients[client_fd]->getNick();
		std::cout << "Client disconnected: " << nickname << " FD " << client_fd << std::endl;
		_clients.erase(client_fd);
	}
	close(client_fd);
}

void	Server::run() // Main server loop
{
	setUpSocket();

	while (true) {
		int ret = poll(_poll_fds.data(), _poll_fds.size(), -1); // Wait activity on any socket
		if (ret < 0)
			throw std::runtime_error("Error: Poll failed.");

		size_t i = 0;
		while (i < _poll_fds.size()) {
			if (_poll_fds[i].revents & POLLIN) {
				if (_poll_fds[i].fd == _server_fd) {// If new connection call acceptNewClient
					acceptNewClient();
					Logger::log(LogLevel::DEBUG, "Active clients " + std::to_string(_clients.size()));
				}
				else
					handleClientInput(_poll_fds[i].fd); // If an existing client call handleInput
			}
			i++;
		}
	}
}

void Server::handleCAP(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;
	
	if (params.size() >= 1 && params[0] == "LS") {
		std::string response = ":ircserv CAP * LS: \r\n";
		send(client->getFd(), response.c_str(), response.length(), 0);
		Logger::log(LogLevel::DEBUG, "Handled CAP LS for FD: " + std::to_string(client->getFd()));
	}
}

// updated handlekick, to take whole ParsedInput struct instead of just params
void Server::handleKICK(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;
	
	if (params.size() < 2) {
		client->sendNumericReply(461, "KICK :Not enough parameters");
		return;
	}
	
	const std::string& channelName = params[0];
	const std::string& targetNick = params[1];
	std::string reason;

	if (params.size() > 2) {
		reason = params[2];
	}
	else {
		reason = client->getNick();
	}

	if (_channels.count(channelName) == 0) {
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}
	
	std::shared_ptr<Channel> channel = _channels.at(channelName);
	
	// check if user is operator
	if (!channel->isOperator(client->getNick())) {
		client->sendNumericReply(482, channelName + " :You're not channel operator");
		return;
	}
	
	// notify channel about kick
	std::string kickMessage = ":" + client->getNick () + " KICK " + channelName + " " + targetNick + " :" + reason;
	channel->broadcast(kickMessage);
	
	// remove user from channel
	channel->removeUser(targetNick);
}
// updated handleinvite, to take whole ParsedInput struct instead of just params
void Server::handleINVITE(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;

	if (params.size() < 2) {
		client->sendNumericReply(461, "INVITE :Not enough parameters");
		return;
	}
	
	const std::string& targetNick = params[0];
	const std::string& channelName = params[1];
	
	if (_channels.count(channelName) == 0) {
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}
	
	std::shared_ptr<Channel> channel = _channels.at(channelName);
	
	// check if user is operator
	if (!channel->isOperator(client->getNick())) {
		client->sendNumericReply(482, channelName + " :You're not channel operator");
		return;
	}
	
	// find target user
	std::shared_ptr<Client> targetClient = nullptr;
	for (const auto& [fd, c] : _clients) {
		if (c->getNick() == targetNick) {
			targetClient = c;
			break;
		}
	}
	
	if (!targetClient) {
		client->sendNumericReply(401, targetNick + " :No such nick/channel");
		return;
	}
	
	// send invite
	channel->inviteUser(client->getNick(), targetNick);
	
	// notify both users
	client->sendNumericReply(341, targetNick + " " + channelName);
	targetClient->sendMessage(":" + client->getNick() + " INVITE " + targetNick + " " + channelName);
}

// updated handletopic, to take whole ParsedInput struct instead of just params
void Server::handleTOPIC(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;

	if (params.empty()) {
		client->sendNumericReply(461, "TOPIC :Not enough parameters");
		return;
	}
	
	const std::string& channelName = params[0];
	
	if (_channels.count(channelName) == 0) {
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}
	
	std::shared_ptr<Channel> channel = _channels.at(channelName);
	
	if (params.size() == 1) {
		// get topic
		std::string topic = channel->getTopic();
		if (topic.empty())
			client->sendNumericReply(331, channelName + " :No topic is set");
		else
			client->sendNumericReply(332, channelName + " :" + topic);
	}
	else {
		// set topic
		const std::string& newTopic = params[1];
		channel->setTopic(client->getNick(), newTopic);
		
		// broadcast topic change
		std::string topicMessage = ":" + client->getNick() + " TOPIC " + channelName + " :" + newTopic;
		channel->broadcast(topicMessage);
		client->sendMessage(topicMessage); // also send to the user who set it
	}
}

// updated handlemode, to take whole ParsedInput struct instead of just params
void Server::handleMODE(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;

	if (params.empty()) {
		client->sendNumericReply(461, "MODE :Not enough parameters");
		return;
	}
	
	const std::string& channelName = params[0];
	
	if (_channels.count(channelName) == 0) {
		client->sendNumericReply(403, channelName + " :No such channel");
		return;
	}
	
	std::shared_ptr<Channel> channel = _channels.at(channelName);
	
	// check if user is operator
	if (!channel->isOperator(client->getNick())) {
		client->sendNumericReply(482, channelName + " :You're not channel operator");
		return;
	}
	
	if (params.size() == 1) {
		// get channel modes (simplified)
		client->sendNumericReply(324, channelName + " +nt");
		return;
	}
	
	const std::string& modeString = params[1];
	size_t paramIndex = 2;
	bool adding = true;
	
	for (char c : modeString) {
		if (c == '+') {
			adding = true;
		}
		else if (c == '-') {
			adding = false;
		}
		else {
			std::string arg = "";
			if ((c == 'k' || c == 'l' || c == 'o') && paramIndex < params.size()) {
				arg = params[paramIndex++];
			}
			
			channel->setMode(c, adding, arg);
			
			// broadcast change of mode
			std::string modeMessage = ":" + client->getNick() + " MODE " + channelName + " " + 
									(adding ? "+" : "-") + c;
			if (!arg.empty())
				modeMessage += " " + arg;
			
			channel->broadcast(modeMessage);
			client->sendMessage(modeMessage);
		}
	}
}

std::vector<std::string> split(const std::string& input, char delimiter)
{
	std::vector<std::string> result;
	std::string line;
	std::stringstream ss(input);
	while(std::getline(ss, line, delimiter))
		result.push_back(line);
	
	return result;
}

bool isValidChannelName(const std::string& name)
{
	if (name.empty())
		return false;

	if (name[0] != '#' && name[0] != '&')
		return false;
	
	if (name.length() > 50)
		return false;
	
	for (char c : name) {
		if (c == ' ' || c == ',' || c < 0x20)
			return false;
	}
	return true;
}

/* handleJOIN:
	- needs a valid channel argument
*/
void	Server::handleJOIN(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;

	if (params.empty()) {
		client->sendNumericReply(461, "JOIN :Not enough parameters");
		return;
	}
	std::vector<std::string> channelNames = split(params[0], ',');
	std::vector<std::string> keys;
	if (params.size() > 1)
		keys = split(params[1], ',');
	
	for (size_t i = 0; i < channelNames.size(); ++i) {
		std::string name = channelNames[i];
		std::string key = (i < keys.size()) ? keys[i] : "";

		if (!isValidChannelName(name)) {
			client->sendNumericReply(476, name + " :Bad channel mask");
			continue;
		}
		std::shared_ptr<Channel> channel;
		auto it = _channels.find(name);

		// create channel if it doesn't exist
		if (it == _channels.end()) {
			channel = std::make_shared<Channel>(name, client->getUser());
			_channels[name] = channel;

			if (!channel->addUser(client->getUser(), key)) {
				client->sendNumericReply(403, name + " :Unable to join channel");
			}
			channel->addOperator(client->getNick());
			
		}
		else {
			channel = it->second;;

			if (channel->isUser(client->getNick())) {
				client->sendNumericReply(443, name + " :is already on channel");
				continue;
			}
			if (!channel->addUser(client->getUser(), key)) {
				if (channel->isInviteOnly() && !channel->isInvited(client->getNick())) {
					client->sendNumericReply(473, name + " :Cannot join channel (+i)");
					continue;
				}
				else if (channel->hasKey() && channel->getKey() != key) {
					client->sendNumericReply(475, name + " :Cannot join channel (+k)");
					continue;
				}
				else if (channel->isFull()) {
					client->sendNumericReply(471, name + " :Cannot join channel (+l)");
					continue;
				}
				else
					client->sendNumericReply(403, name + " :Unable to join channel");
			}
			channel->removeInvite(client->getNick());
		}
		// Send JOIN reply
		client->sendMessage(":" + client->getUser()->getPrefix() + " JOIN " + name + "\r\n");

		// Send topic
		if (!channel->getTopic().empty())
			client->sendNumericReply(332, name + " :" + channel->getTopic());

		// Send names list
		std::string namesReply = channel->getUserListWithPrefixes();
		client->sendNumericReply(353, "= " + name + " :" + namesReply);
		client->sendNumericReply(366, name + " :End of /NAMES list");

		channel->broadcast(":" + client->getUser()->getPrefix() + " JOIN " + name + "\r\n", client->getUser()->getNickname());
	}
}

//void	Server::handlePART(std::shared_ptr<Client> client, const ParsedInput& parsed)
//{}

void	Server::handleNICK(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;
	
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
	
	if (!client->getUser()->isValidNickname(newNick))
	{
		client->sendNumericReply(432, newNick + ":Invalid nickname");
		return;
	}

	for (const auto& [fd, otherClient] : _clients)
	{
		if (otherClient->getUser()->getNickname() == newNick)
		{
			client->sendNumericReply(433, newNick + " :Nickname is already in use");
			return;
		}
	}

	client->getUser()->setNickname(newNick);
	Logger::log(LogLevel::DEBUG, "Set nickname to " + newNick + " for FD: " + std::to_string(client->getFd()));

	/*if (client->isAuthenticated() && client->isRegistered()) 
	{
		client->sendNumericReply(001, ":Welcome to the IRC server" + client->getNick());
		client->sendNumericReply(002, ":Your host is ircserver, running version ft_irc");
		client->sendNumericReply(003, ":This server was created" + client->getCurrentDate());
		client->sendNumericReply(004, ":irc.server.com ft_irc <suppoted user modes> <supported channel modes>");
	}*/

	client->tryRegister(client->getUser()->getNickname(), client->getUser()->getUsername(), client->getUser()->getRealname());
}

void	Server::handleUSER(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;

	if (!client->isAuthenticated())
	{
		client->sendNumericReply(451, "USER :You have not registered (missing PASS)");
		return;
	}
	if (params.size() < 4 || params[3].empty() || params[3][0] != ':')
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

	if (!client->getUser()->isValidUsername(username))
	{
		client->sendNumericReply(468, ":Invalid username");
		return;
	}

	if (!client->getUser()->isValidRealname(realname))
	{
		client->sendNumericReply(468, ":Invalid realname");
		return;
	}

	client->getUser()->setUsername(username);
	client->getUser()->setRealname(realname);

	client->setAuthenticated(true);

		Logger::log(LogLevel::DEBUG, "Set username to " + username + ", realname to " + realname 
			+ " for FD: "+ std::to_string(client->getFd()));

	if (client->isAuthenticated() && client->isRegistered()) 
	{
		client->sendNumericReply(001, ":Welcome to the IRC server" + client->getNick());
		client->sendNumericReply(002, ":Your host is ircserver, running version ft_irc");
		client->sendNumericReply(003, ":This server was created" + client->getCurrentDate());
		client->sendNumericReply(004, ":irc.server.com ft_irc <suppoted user modes> <supported channel modes>");
	}
}

void	Server::handlePASS(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;

	if (params.empty())
	{
		client->sendNumericReply(461, " PASS: Not enough parameters");
		return ;
	}
	Logger::log(LogLevel::DEBUG, "Expected password " + _password);
	Logger::log(LogLevel::DEBUG, "Received password " + params[0]);

	std::string received = params[0];
	received.erase(received.find_last_not_of("\r\n") + 1);

	Logger::log(LogLevel::DEBUG, "Expected password: [" + _password + "]");
	Logger::log(LogLevel::DEBUG, "Received password: [" + received + "]");

	if (client->isAuthenticated())
	{
		client->sendNumericReply(462, ":You may not re-register");
		return;
	}

	if (params.empty())
	{
		client->sendNumericReply(461, "PASS :Not enough parameters");
		return;
	}

	if (received != _password)
	{
		client->sendNumericReply(464, ":Password incorrect");
		return;
	}

	client->setAuthenticated(true);
}

void	Server::handlePRIVMSG(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;

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

		std::shared_ptr<Channel> channel = _channels.at(receiver);
		if (!channel->hasUser(client->getUser()->getNickname()))
		{
			client->sendNumericReply(404, receiver + " :Cannot send to channel");
			return;
		}

		std::string fullMsg = ":" + client->getUser()->getNickname() + " PRIVMSG " + receiver + " :" + message;
		channel->broadcast(fullMsg, client->getUser()->getNickname());
	}
	else //privmsg to another user
	{
		std::shared_ptr<Client> target = nullptr;
		Logger::log(LogLevel::DEBUG, "PRIVMSG from: [" + client->getUser()->getNickname() + "] to [" + receiver + "]");

		for (const auto& [fd, otherClient] : _clients) {
			Logger::log(LogLevel::DEBUG, "Checking against registered user: [" + otherClient->getUser()->getNickname() + "]");
			if (otherClient->getUser()->getNickname() == receiver) {
				target = otherClient;
				break;
			}
		}

		if (!target)
		{
			client->sendNumericReply(401, receiver + " :No such nick");
			return;
		}

		std::string fullMsg = ":" + client->getUser()->getNickname() + " PRIVMSG " + receiver + " :" + message;
		Logger::log(LogLevel::DEBUG, "Sending message to " + target->getUser()->getNickname() + ":" + fullMsg );
		target->sendMessage(fullMsg);
	}
}

void	Server::handleNOTICE(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;

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

		std::shared_ptr<Channel> channel = _channels.at(receiver);
		if (!channel->hasUser(client->getUser()->getNickname()))
			return;

		std::string fullMsg = ":" + client->getUser()->getNickname() + " NOTICE " + receiver + " :" + message;
		channel->broadcast(fullMsg, client->getUser()->getNickname());
	}
	else
	{
		for (const auto& [fd, otherClient] : _clients)
		{
			if (otherClient->getUser()->getNickname() == receiver)
			{
				std::string fullMsg = ":" + client->getUser()->getNickname() + " NOTICE " + receiver + " :" + message;
				otherClient->sendMessage(fullMsg);
				break;
			}
		}
	}
}

void	Server::handleQUIT(std::shared_ptr<Client> client, const ParsedInput& parsed)
{
	const std::vector<std::string>& params = parsed.parameters;
	//not fully tested
	std::string quitMsg = "Client Quit";
	if (!params.empty()) {
		quitMsg = params[0];
		if (quitMsg[0] == ':')
			quitMsg = quitMsg.substr(1);
	}

	std::string nickname = client->getUser()->getNickname();
	std::string fullMsg = ":" + nickname + "Quit :" + quitMsg;

	std::vector<std::string> affectedChannels;
	for (const auto& [name, channel] : _channels) {
		if (channel->hasUser(nickname)) {
			channel->broadcast(fullMsg, nickname);
			affectedChannels.push_back(name);
		}
	}

	for (const std::string& name : affectedChannels)
		_channels[name]->removeUser(nickname);

	Logger::log(LogLevel::DEBUG, "QUIT:" + fullMsg);
	client->sendMessage(fullMsg);
	close(client->getFd());
	_clients.erase(client->getFd());
}
