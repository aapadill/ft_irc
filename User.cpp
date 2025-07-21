/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/06 16:00:24 by nmeintje          #+#    #+#             */
/*   Updated: 2025/06/06 16:00:27 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "User.hpp"

User::User(std::string nick, int sock) : _nickname(std::move(nick)), _socket(sock), _authenticated(false), _registered(false)
{
    // should we initialize these? 
    _username = "";
    _realname = "";
    _buffer.clear(); // empty buffer?
}

std::string User::getNickname() const
{ 
    return _nickname;
}

std::string User::getUsername() const
{
    return _username;
}

std::string User::getRealname() const
{
    return _realname;
}

int User::getSocket() const
{
    return _socket;
}

bool User::isAuthenticated() const
{
    return _authenticated;
}

bool User::isRegistered() const
{
    return _registered;
}

void	User::checkRegisteration()
{
	if (isAuthenticated() && !isRegistered() && !_nickname.empty() && !_username.empty())
	{
		setRegistered(true);
		sendNumericReply(001, ":Welcome to the IRC Server " + _nickname);
		sendNumericReply(002, ":Your host is ircserver, running version ft_irc");
		sendNumericReply(003, ":This server was created " + getCurrentDate());
		sendNumericReply(004, ":irc.server.com ft_irc <supported user modes> <supported channel modes>");
	}
}

void User::setNickname(const std::string& nick)
{
    _nickname = nick;
}

void User::setUsername(const std::string& user)
{
    _username = user;
}

void User::setRealname(const std::string& real)
{
    _realname = real;
}

void User::setAuthenticated(bool auth)
{
    _authenticated = auth;
}

void User::setRegistered(bool reg)
{
    _registered = reg;
}

void User::appendToBuffer(const std::string& data)
{
    _buffer += data;
}

//std::string User::extractFromBuffer() {}

void User::sendMessage(const std::string& message) 
{
    // send message to socket_
    //std::cout << "To [" << _nickname << "]: " << message << "\n";
	std::string msgWithCRLF = message + "\r\n";
	send(_socket, msgWithCRLF.c_str(), msgWithCRLF.size(), 0);
}

void User::sendNumericReply(int code, const std::string& message)
{
	std::string reply = ":" + std::string("irc.server.com") + " " + //or localhost?
						std::to_string(code) + " " + _nickname + " " + message;
	sendMessage(reply);
}

std::string	User::getCurrentDate() const
{
	std::time_t now = std::time(nullptr);
	std::tm* localTime = std::localtime(&now);

	std::ostringstream oss;
	oss << std::asctime(localTime);

	std::string dateStr = oss.str();
	dateStr.erase(dateStr.find_last_not_of("\n") + 1);
	return dateStr;
}

bool	User::isValidNickname(const std::string& nickname)
{
	if (nickname.empty() || isdigit(nickname[0] || nickname[0] == '-'))
		return false;
	if (nickname.length() > 9)
		return false;
	for (char c : nickname)
	{
		if (!isalnum(c) && std::string("-[]\\`^^{}").find(c) == std::string::npos)
			return false;
	}
	return true;
}

bool	User::isValidUsername(const std::string& username)
{
	if (username.empty())
		return false;
	for (char c : username)
	{
		if (!isalnum(c) && c != '_')
			return false;
	}
	return true;
}

bool	User::isValidRealname(const std::string& realname)
{
	if (realname.empty())
		return false;

	for (char c : realname)
	{
		if (iscntrl(c))
			return false;
		if (!isalpha(c) && c != ' ')
			return false;
	}
	return true;
}
