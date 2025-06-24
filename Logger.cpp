/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/24 11:54:10 by nmeintje          #+#    #+#             */
/*   Updated: 2025/06/24 11:54:12 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logger.hpp"

Logger::~Logger() {}

std::string current_time()
{
    auto    now = std::chrono::system_clock::now();
    auto    t = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
    localtime_r(&t, &tm);
    std::ostringstream  oss;

    oss <<std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void    Logger::log(LogLevel level, const std::string& msg)
{
    std::string levelStr;
    std::string color;

    switch (level) {
        case LogLevel::INFO:
            levelStr = "[ INFO ]";
            color = GREEN;
            break ;
        case LogLevel::WARNING:
            levelStr = "[ WARNING ]";
            color = YELLOW;
            break ;
        case LogLevel::ERROR:
            levelStr = "[ ERROR ]";
            color = RED;
            break ;
        case LogLevel::DEBUG:
            levelStr = "[ DEBUG ]";
            color = CYAN;
            break ;
        default:
            levelStr = "LOG";
            color = DEFAULT;
    }
    std::cout <<  "[" << current_time() << "] " << color << levelStr << DEFAULT
              << ": " << msg << std::endl;
}
