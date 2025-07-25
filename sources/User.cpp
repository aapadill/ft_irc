/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/06 16:00:24 by nmeintje          #+#    #+#             */
/*   Updated: 2025/06/06 16:00:27 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "User.hpp"
#include "Client.hpp"

User::User(std::string nick) : _nickname(std::move(nick))
{
    _username = "";
    _realname = "";
}

User::User(Client* client) : _client(client) {}

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

std::string User::getPrefix() const
{
    return _nickname + "!" + _username + "@localhost";
}

void User::sendMessage(const std::string& message)
{
	if (_client)
		_client->sendMessage(message);
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
