/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:08:05 by nmeintje          #+#    #+#             */
/*   Updated: 2025/07/02 14:08:07 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>

enum class CommandType {
    PASS,
    NICK,
    USER,
    CAP,
    PING,
    PONG,
    QUIT,
    WHOIS,
    WHO,
    KICK,
    INVITE,
    TOPIC,
    MODE,
    JOIN,
    PART,
    PRIVMSG,
    NOTICE,
    UNKNOWN
};

CommandType stringToCommand(const std::string& cmd);
std::string commmandToString(CommandType cmd);