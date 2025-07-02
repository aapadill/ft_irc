/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/06 15:59:55 by nmeintje          #+#    #+#             */
/*   Updated: 2025/06/06 15:59:56 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"
#include "User.hpp"

Channel::Channel(const std::string& name) : _name(name) {}

Channel::~Channel(void) {}

bool Channel::addUser(std::shared_ptr<User> user, const std::string& providedKey) 
{
    const std::string& nick = user->getNickname();

    if (_inviteOnly && !isInvited(nick))
        return false;

    if (_hasKey && providedKey != _key)
        return false;

    if (_hasUserLimit && _users.size() >= _userLimit)
        return false;

    _users[nick] = user;
    _invited.erase(nick);
    broadcast(nick + " has joined the channel");

    return true;
}

void Channel::removeUser(const std::string& nickname)
{
    _users.erase(nickname);
    _operators.erase(nickname);
    broadcast(nickname + " has left the channel");
}

void Channel::addOperator(const std::string& nickname)
{
    if (_users.count(nickname)) {
        _operators.insert(nickname);
    }
}

void Channel::removeOperator(const std::string& nickname)
{
    _operators.erase(nickname);
}

bool Channel::isOperator(const std::string& nickname) const 
{
    return _operators.count(nickname);
}

void Channel::setTopic(const std::string& nickname, const std::string& newTopic)
{
    if (_topicRestricted && !isOperator(nickname)) return;
    _topic = newTopic;
    broadcast("Topic set to: " + _topic);
}

std::string Channel::getTopic(void) const
{
    return _topic;
}

void Channel::inviteUser(const std::string& by, const std::string& target) 
{
    if (isOperator(by)) {
        _invited.insert(target);
    }
}

bool Channel::isInvited(const std::string& nickname) const
{
    return _invited.count(nickname);
}

void Channel::setMode(char mode, bool enable, const std::string& arg) 
{
    switch (mode) {
        case 'i':
            _inviteOnly = enable;
            break;
        case 't':
            _topicRestricted = enable;
            break;
        case 'k':
            if (enable && !arg.empty()) {
                _hasKey = true;
                _key = arg;
            } else {
                _hasKey = false;
                _key.clear();
            }
            break;
        case 'l':
            if (enable && !arg.empty()) {
                _hasUserLimit = true;
                _userLimit = static_cast<size_t>(std::stoi(arg));
            } else {
                _hasUserLimit = false;
                _userLimit = 0;
            }
            break;
       case 'o':
            if (!arg.empty()) {
                if (enable)
                    addOperator(arg);
                else
                    removeOperator(arg);
            }
            break;
        default:
            break;
    }
}

void Channel::broadcast(const std::string& message, const std::string& sender)
{
    for (auto& [nick, user] : _users) {
        if (nick != sender)
            user->sendMessage("[" + _name + "] " + message);
    }
}

std::string Channel::getName(void) const
{
    return _name;
}
