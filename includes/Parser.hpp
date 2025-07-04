#ifndef PARSER_HPP
# define PARSER_HPP

# include <string>
# include <vector>
# include <optional>
# include <sstream>
# include <algorithm>
# include <cctype>
# include <iostream>
# include <regex>
# include <unordered_set>

#include "Command.hpp"

struct	ParsedInput // IRC message may consist of up to three main parts: the prefix (OPTIONAL), the command, and the command parameters (maximum of 15). The prefix, command and all parameters are separated by one ASCII space character.
{
	std::optional<std::string> prefix; // Preference of prefix is indicated with a single leading ASCII colon character ':', which must be the first character of the essage itself. There must be no gap (whitespace) between the colon and the prefix. The prefix is used by servers to indicate the true origin of the message. If the prefix is missing from the messagem it is assumed to have originated from the connection from which it was received from. HOX! Clients should not use a prefix when sending a message; if they use one, the only valid prefix is the registered nickname associated with the clien.
	std::string command; // The command must either be a valid IRC command or a three digit number represented in ASCII text.
	std::vector<std::string> parameters; // IRC messaged are always lines of characters terminated with a CR-LF (Carriage Return - Line Feed) pair, and these messages shall not exceed 512 characters in lenght, counting all characters including the trailing CR-LF. Thus there are 510 characters maximum allowed for the command and its parameters.

	CommandType commandType = CommandType::UNKNOWN;
};

class	Parser
{
	private:
		
		Parser(Parser const &copy) = delete;
		Parser &operator=(Parser const &copy) = delete;

	public:
		Parser();
		~Parser();

		std::optional<ParsedInput> parse(std::string const &input);
};

std::optional<std::string> extractPrefix(const std::string &line, size_t &pos);
std::string extractCommand(const std::string &line, size_t &pos);
std::vector<std::string> extractParameters(const std::string &line, size_t &pos);

#endif
