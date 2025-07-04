#include "Parser.hpp"

Parser::Parser() {}

Parser::~Parser() {}

std::optional<ParsedInput> Parser::parse(std::string const &input)
{
	if (input.empty() || input.size() > 512 || std::isspace(input[0]))
		return std::nullopt; // std::optional<T> return for error

	std::string line = input;
	if (line.size() >= 2 && line.substr(line.size() - 2) == "\r\n")
		line = line.substr(0, line.size() - 2);

	ParsedInput result;
	size_t pos = 0;

	if (line[0] == ':')
	{
		auto prefix = extractPrefix(line, pos);
		if (!prefix)
			return std::nullopt;
		result.prefix = *prefix;
	}
		
	result.command = extractCommand(line, pos);
	std::transform(result.command.begin(), result.command.end(), result.command.begin(), ::toupper);

	result.commandType = stringToCommand(result.command);
	if (result.commandType == CommandType::UNKNOWN)
		return std::nullopt;

	result.parameters = extractParameters(line, pos);
	if (result.parameters.size() > 15)
		return std::nullopt;
	
	return result;
}

bool isValidPrefix(const std::string &prefix)
{
	std::regex PrefixRegex(R"(^([A-Za-z\[\]\\`_^{|}][-A-Za-z0-9\[\]\\`_^{|}]*)((![^@\s]+)@([^\s]+))?$|^[a-zA-Z0-9.-]+$)");
	return std::regex_match(prefix, PrefixRegex);
}

std::optional<std::string> extractPrefix(const std::string &line, size_t &pos)
{
	size_t space = line.find(' ', pos);
	if (space == std::string::npos)
		return std::nullopt;
	
	std::string prefix_str = line.substr(pos + 1, space - 1);
	if (isValidPrefix(prefix_str))
		return std::nullopt;
	
	pos = space + 1;
	if (line[pos] == ' ')
		return std::nullopt;

	return prefix_str;

}

std::string extractCommand(const std::string &line, size_t &pos)
{
	size_t space = line.find(' ', pos);
	std::string cmd;

	if (space == std::string::npos) {
		cmd = line.substr(pos);
		pos = line.size();
	} else {
		cmd = line.substr(pos, space - pos);
		pos = space + 1;
	}
	return cmd;
}

std::vector<std::string> extractParameters(const std::string &line, size_t &pos)
{
	std::vector<std::string> parameters;

	while (pos < line.length())
	{
		if (line[pos] == ' ')
			break;

		if (parameters.size() == 14 && line[pos] != ':')
			break;

		if (line[pos] == ':')
		{
			parameters.push_back(line.substr(pos + 1));
			break ;
		}

		size_t next_space = line.find(' ', pos);
		if (next_space == std::string::npos)
		{
			parameters.push_back(line.substr(pos));
			break;
		}
		parameters.push_back(line.substr(pos, next_space - pos));
		pos = next_space + 1;
	}
	return parameters;
}
