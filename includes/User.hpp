/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.hpp                                         :+:      :+:    :+:   */
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
#include <ctime>
#include <sstream>
#include <memory>

class Client;

class User {

    private:
        std::string _nickname;
        std::string _username;
        std::string _realname;

        Client* _client;
        
    public:
    
        User(std::string nick);
        User(Client* client);

        Client* getClient() const;
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getRealname() const;
        std::string getPrefix() const;

        void setNickname(const std::string& nick);
        void setUsername(const std::string& user);
        void setRealname(const std::string& real);

        void appendToBuffer(const std::string& data);
        std::string extractFromBuffer();
        bool hasCompleteMessage() const;

        void sendMessage(const std::string& message);
       
		bool	isValidNickname(const std::string& nickname);
		bool	isValidUsername(const std::string& username);
		bool	isValidRealname(const std::string& realname);
};
