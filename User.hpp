/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/06 16:00:42 by nmeintje          #+#    #+#             */
/*   Updated: 2025/06/06 16:00:44 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <iostream>
#include <sys/socket.h>

class User {

    private:
        std::string _nickname;
        std::string _username;
        std::string _realname;
        int         _socket;
        bool        _authenticated;
        bool        _registered;
        std::string _buffer; //helpful to have the incoming data until a complete message is formed
    public:
    
        User(std::string nick, int sock);
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getRealname() const;

        int getSocket() const;
        bool isAuthenticated() const;
        bool isRegistered() const;

        void setNickname(const std::string& nick);
        void setUsername(const std::string& user);
        void setRealname(const std::string& real);
        void setAuthenticated(bool auth);
        void setRegistered(bool reg);

        void appendToBuffer(const std::string& data);
        std::string extractFromBuffer();
        bool hasCompleteMessage() const;

        void sendMessage(const std::string& message);
        void sendNumericReply(int code, const std::string& message);
};
