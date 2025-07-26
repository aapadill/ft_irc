#include "Parser.hpp"

Parser::Parser() {}

Parser::~Parser() {}

std::optional<ParsedInput> Parser::parse(std::string const &input)
{
	if (input.empty() || input.size() > 512)
		return std::nullopt; // std::optional<T> return for error

	std::string line = input;

	if (std::isspace(line[0]))
		return std::nullopt;
	
	/*if (line.size() >= 2 && line.substr(line.size() - 2) == "\r\n")
		line = line.substr(0, line.size() - 2);*/
	// segfault fixed when empty line
	size_t trim_pos = line.find_last_not_of("\r\n");
	if (trim_pos != std::string::npos)
		line.erase(trim_pos + 1);
	else
		line.clear();


	ParsedInput result;
	size_t pos = 0;

	if (line[0] == ':')
	{
		size_t space = line.find(' ');
		if (space == std::string::npos)
			return std::nullopt;

		std::string prefix_str = line.substr(1, space - 1);
		std::regex PrefixRegex(R"(^([A-Za-z\[\]\\`_^{|}][-A-Za-z0-9\[\]\\`_^{|}]*)((![^@\s]+)@([^\s]+))?$|^[a-zA-Z0-9.-]+$)");
		if (!std::regex_match(prefix_str, PrefixRegex))
			return std::nullopt;

		result.prefix = prefix_str;
		pos = space + 1;
		if (line[pos] == ' ')
			return std::nullopt;
	}
		
	size_t space = line.find(' ', pos);
	if (space == std::string::npos)
	{
		result.command = line.substr(pos);
		pos = line.length();
	}
	else
	{
		result.command = line.substr(pos, space - pos);
		pos = space + 1;
	}
	std::transform(result.command.begin(), result.command.end(), result.command.begin(), ::toupper);

	/*static const std::unordered_set<std::string> validCommand = { "PASS", "NICK", "USER", "JOIN", "PART", "PRIVMSG", "NOTICE", "QUIT", "KICK", "INVITE", "TOPIC", "MODE" };
	if (validCommand.find(result.command) == validCommand.end())
		return nullopt;*/

	while (pos < line.length())
	{
		if (pos >= line.length())
			break;
		if (pos < line.length() && line[pos] == ' ')
			return std::nullopt;

		if (result.parameters.size() == 14 && line[pos] != ':')
			return std::nullopt;

		if (line[pos] == ':')
		{
			result.parameters.push_back(line.substr(pos + 1));
			break ;
		}

		size_t next_space = line.find(' ', pos);
		if (next_space == std::string::npos)
		{
			result.parameters.push_back(line.substr(pos));
			break;
		}
		result.parameters.push_back(line.substr(pos, next_space - pos));
		pos = next_space + 1;
	}

	for (size_t i = 0; i < result.parameters.size(); ++i)
	{
		// segfault fixed when empty line
		if (!result.parameters[i].empty()) {
			size_t param_pos = result.parameters[i].find_last_not_of("\r\n");
			if (param_pos != std::string::npos)
				result.parameters[i].erase(param_pos + 1);
			else
				result.parameters[i].clear();
		}
	}

	return result;
}
