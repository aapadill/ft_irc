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

std::string User::getPrefix() const
{
    return _nickname + "!" + _username + "@localhost";
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