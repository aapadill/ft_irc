#ifndef SERVER_HPP
# define SERVER_HPP

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
# include <sstream>
# include "Parser.hpp"
# include "User.hpp"
# include "Channel.hpp"

#define YELLOW "\033[33m"
#define RED "\033[31m"
#define DEFAULT "\033[0;0m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"
#define CYAN "\033[1;36m"
#define MAGENTA "\033[1;35m"

class User;
class Channel;
class Parser;
class Client;
struct ParsedInput;

#define SERVER_CHANNEL_LIMIT 50
#define USER_CHANNEL_LIMIT 10
#define SERVER_USER_LIMIT 1000

class Server
{
	private:
		int _port; // Port number that server listens.
		std::string _password; // Password required to connect
		int	_server_fd; // File descriptor for the main server socket
		std::map<int, std::shared_ptr<Client>> _clients; // Stores connected clients using their socket file descriptor as the key
		std::map<std::string, std::shared_ptr<Channel>> _channels; // Maps channel names to Channel objects
		std::vector<struct pollfd> _poll_fds; // Monitoring multiple socket FDs

		Parser* _parser;

		Server(Server const &copy) = delete;
		Server &operator=(Server const &copy) = delete;

		void	setUpSocket();
		void	acceptNewClient();
		void	handleClientInput(int client_fd);
		void	removeClient(int client_fd);
		void	dispatchCommand(std::shared_ptr<Client> client, ParsedInput const &parsed);

		//commands //kick, invite, topic, mode (i, t, k, o, l)
		
		void	handleKICK(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleINVITE(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleTOPIC(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleMODE(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleJOIN(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleCAP(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handlePART(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleNICK(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleUSER(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handlePASS(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handlePRIVMSG(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleNOTICE(std::shared_ptr<Client> client, const ParsedInput& parsed);
		void	handleQUIT(std::shared_ptr<Client> client, const ParsedInput& parsed);

	public:
		Server(int port, std::string const &password);
		~Server();

		void	run();
};


#endif
