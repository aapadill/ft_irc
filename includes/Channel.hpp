/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/06 16:00:04 by nmeintje          #+#    #+#             */
/*   Updated: 2025/06/06 16:00:06 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>

class User;

class Channel {

	private:
		
		std::string _name;
		std::string _topic;
		bool _hasKey = false; // for if channel has a password
		std::string _key;
		bool _hasUserLimit = false;
		size_t _userLimit = 0;
		bool _inviteOnly = false;
		bool _topicRestricted = false;

		std::unordered_map<std::string, std::shared_ptr<User>>  _users;
		std::unordered_set<std::string>                         _operators;
		std::unordered_set<std::string>                         _invited;
		
	public:
		//constructors and destructor
		Channel(void) = delete;
		Channel(const Channel& copy) = delete;
		Channel(const std::string& name);
		Channel(const std::string& name, std::shared_ptr<User> user);
		Channel &operator=(const Channel& other) = delete;
		~Channel(void);

		// methods related to users
		bool addUser(std::shared_ptr<User> user, const std::string& providedKey = "");
		void removeUser(const std::string& nickname);
		bool isUser(const std::string& nickname) const;
		std::string getUserListWithPrefixes() const;
		bool	hasUser(const std::string& nickname) const;

		// methods related to operators
		void addOperator(const std::string& nickname);
		void removeOperator(const std::string& nickname);
		bool isOperator(const std::string& nickname) const;

		// Channel topics
		void setTopic(const std::string& nickname, const std::string& newTopic);
		std::string getTopic(void) const;

		// for if channel is invite only/private
		void inviteUser(const std::string& by, const std::string& target);
		bool isInvited(const std::string& nickname) const;
		bool isInviteOnly() const;
		void removeInvite(const std::string& nickname);

		// passwords
		bool hasKey() const;
		const std::string& getKey() const;
		void setKey(const std::string& key);

		void setMode(char mode, bool enable, const std::string& arg = "");
		void broadcast(const std::string& message, const std::string& sender = "");
		std::string getName(void) const;
		bool isFull() const;
};
