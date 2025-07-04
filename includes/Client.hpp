/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/04 13:56:41 by nmeintje          #+#    #+#             */
/*   Updated: 2025/07/04 13:56:43 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sstream>

class User;

class Client {

	private:
		int 		_fd;
		std::string	_recvBuffer;
		bool		_hasNick;
		bool		_hasUser;
		std::shared_ptr<User>	_user;
		User		_user;

		Client(const Client& copy) = delete;
		
	public:
		Client();
		Client(int fd);
		Client &operator=(const Client& other);
		~Client();

		int getFd() const;
		void appendToBuffer(const std::string& data);
		bool hasCompleteMessage() const;
		std::string getNextMessage(); //split by r/n/

		bool isRegistered() const;
		void markHasNick();
		void markHasUser();
		void tryRegister(const std::string& nick, const std::string& user, const std::string& real);

		std::shared_ptr<User> getUser();
		void sendMessage(const std::string& msg);
		void sendNumericReply(int code, const std::string &message);
		std::string getNick() const;
};
		

