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

User::User(std::string nick, int sock) : _nickname(std::move(nick)), _socket(sock) {}

std::string User::getNickname() const
{ 
    return _nickname;
}

void User::sendMessage(const std::string& message) 
{
        // send message to socket_
        //std::cout << "To [" << _nickname << "]: " << message << "\n";
		std::string msgWithCRLF = message + "\r\n";
		send(_socket, msgWithCRLF.c_str(), msgWithCRLF.size(), 0);
}
