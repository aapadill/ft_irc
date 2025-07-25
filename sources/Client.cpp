/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/04 13:59:05 by nmeintje          #+#    #+#             */
/*   Updated: 2025/07/04 13:59:06 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "User.hpp"

Client::Client() : _fd(-1), _recvBuffer(""), _hasNick(false), _hasUser(false), _user(nullptr), 
_buffer(), _registered(false), _authenticated(false) {}

Client::Client(int fd) : _fd(fd), _recvBuffer(""), _hasNick(false), _hasUser(false), _user(nullptr),
_buffer(), _registered(false), _authenticated(false) 
{
    _user = std::make_shared<User>("");
}

Client &Client::operator=(const Client& other)
{
    if (this != &other) {
        _fd = other._fd;
        _recvBuffer = other._recvBuffer;
        _hasNick = other._hasNick;
        _hasUser = other._hasUser;
        _user = other._user;
    }
    return *this;
}

Client::~Client()
{
    if (_fd != -1) {
        close(_fd);
    }
}

int Client::getFd() const
{
    return _fd;
}

void Client::appendToBuffer(const std::string& data)
{
    _recvBuffer += data;
}

bool Client::hasCompleteMessage() const
{
    return _recvBuffer.find("\r\n") != std::string::npos;
}

std::string Client::getNextMessage()
{
    size_t pos = _recvBuffer.find("\r\n");
    if (pos == std::string::npos)
        return "";

    std::string message = _recvBuffer.substr(0, pos);
    _recvBuffer.erase(0, pos + 2);
    return message;
}

bool Client::isAuthenticated() const
{
    return _authenticated;
}

bool Client::isRegistered() const
{
    return _hasNick && _hasUser;
}

void Client::markHasNick()
{
    _hasNick = true;
}

void Client::markHasUser()
{
    _hasUser = true;
}

void Client::tryRegister(const std::string& nick, const std::string& user, const std::string& real)
{
    if (_hasNick && _hasUser && !_user) {
        _user = std::make_shared<User>(nick);
        _user->setUsername(user);
        _user->setRealname(real);
    }

    sendNumericReply(001, nick + " :Welcome to the IRC network");
}

std::shared_ptr<User> Client::getUser()
{
    return _user;
}

void Client::sendMessage(const std::string& msg)
{
    std::string formatted = msg + "\r\n";
    if (send(_fd, formatted.c_str(),formatted.length(), 0) == -1) {
        std::cerr << "Send error on fd " << _fd << ": " << strerror(errno) << std::endl;
    }
}

std::string Client::getNick() const
{
    return _user->getNickname();
}

void Client::sendNumericReply(int code, const std::string &message)
{
    std::string serverName = SERVER;
    std::string nickname = _user ? _user->getNickname() : "*";

	std::ostringstream oss;
	oss << ":" << serverName << " " <<std::setw(3) << std::setfill('0')
        << code << nickname << " " << message << "\r\n";

	sendMessage(oss.str()); 
}


void Client::setAuthenticated(bool auth)
{
    _authenticated = auth;
}

void Client::setRegistered(bool reg)
{
    _registered = reg;
}

//std::string User::extractFromBuffer() {}

std::string	Client::getCurrentDate() const
{
	std::time_t now = std::time(nullptr);
	std::tm* localTime = std::localtime(&now);

	std::ostringstream oss;
	oss << std::asctime(localTime);

	std::string dateStr = oss.str();
	dateStr.erase(dateStr.find_last_not_of("\n") + 1);
	return dateStr;
}