/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:07:42 by nmeintje          #+#    #+#             */
/*   Updated: 2025/07/02 14:07:43 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Command.hpp"

CommandType stringToCommand(const std::string& cmd)
{
    static const std::unordered_map<std::string, CommandType> commandMap = {
        {"PASS", CommandType::PASS},
        {"NICK", CommandType::NICK},
        {"USER", CommandType::USER},
        {"CAP", CommandType::CAP},
        {"PING", CommandType::PING},
        {"PONG", CommandType::PONG},
        {"QUIT", CommandType::QUIT},
        {"WHOIS", CommandType::WHOIS},
        {"WHO", CommandType::WHO},
        {"KICK", CommandType::KICK},
        {"INVITE", CommandType::INVITE},
        {"TOPIC", CommandType::TOPIC},
        {"MODE", CommandType::MODE},
        {"JOIN", CommandType::JOIN},
        {"PART", CommandType::PART},
        {"PRIVMSG", CommandType::PRIVMSG},
        {"NOTICE", CommandType::NOTICE},
    };
    std::string upperCmd = cmd;
    std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

    if (commandMap.count(upperCmd))
        return commandMap.at(upperCmd);
    return CommandType::UNKNOWN;
}

std::string commmandToString(CommandType cmd)
{
    switch (cmd) {
        case CommandType::PASS: return "PASS";
        case CommandType::NICK: return "NICK";
        case CommandType::USER: return "USER";
        case CommandType::CAP: return "CAP";
        case CommandType::PING: return "PING";
        case CommandType::PONG: return "PONG";
        case CommandType::QUIT: return "QUIT";
        case CommandType::WHOIS: return "WHOIS";
        case CommandType::WHO: return "WHO";
        case CommandType::KICK: return "KICK";
        case CommandType::INVITE: return "INVITE";
        case CommandType::TOPIC: return "TOPIC";
        case CommandType::MODE: return "MODE";
        case CommandType::JOIN: return "JOIN";
        case CommandType::PART: return "PASS";
        case CommandType::PRIVMSG: return "PASS";
        case CommandType::NOTICE: return "PASS";
        default: return "UNKNOWN";

    }
}