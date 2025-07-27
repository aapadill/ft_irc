#ifndef SERVER_HPP
# define SERVER_HPP

// set to false to disable debug output
#define DEBUG_MODE 1

# include <string>
# include <iostream>
# include <cstring>
# include <vector>
# include <map>
# include <optional>
# include <poll.h>
# include <netinet/in.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include "Parser.hpp"
# include "User.hpp"
# include "Channel.hpp"

class User;
class Channel;
class Parser;
struct ParsedInput;

class Server
{
	private:
		int _port; // Port number that server listens.
		std::string _password; // Password required to connect
		int	_server_fd; // File descriptor for the main server socket
		std::map<int, std::shared_ptr<User>> _clients; // Stores connected clients using their socket file descriptor as the key
		std::map<std::string, Channel> _channels; // Maps channel names to Channel objects
		std::vector<struct pollfd> _poll_fds; // Monitoring multiple socket FDs

		Parser* _parser;

		Server(Server const &copy) = delete;
		Server &operator=(Server const &copy) = delete;

		void	setUpSocket();
		void	acceptNewClient();
		void	handleClientInput(int client_fd);
		void	removeClient(int client_fd);
		void	dispatchCommand(std::shared_ptr<User> client, ParsedInput const &parsed);

		//commands //kick, invite, topic, mode (i, t, k, o, l)
		void	handleKICK(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleINVITE(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleTOPIC(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleMODE(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleNICK(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleUSER(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handlePASS(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handlePRIVMSG(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleNOTICE(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleJOIN(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handlePART(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleQUIT(std::shared_ptr<User> client, const std::vector<std::string>& params);
		void	handleHELP(std::shared_ptr<User> client, const std::vector<std::string>&);

		// helpers
		std::shared_ptr<User>	findUserByNick(const std::string& nickname);
		bool					requireRegistration(std::shared_ptr<User> client, const std::string& command);
		Channel*				getChannelIfExists(const std::string& channelName, std::shared_ptr<User> client);
		bool					requireChannelOperator(Channel& channel, std::shared_ptr<User> client, const std::string& channelName);

	public:
		Server(int port, std::string const &password);
		~Server();

		void	run();
};

#endif
