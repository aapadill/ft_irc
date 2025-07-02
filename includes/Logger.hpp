/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmeintje <nmeintje@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/24 11:53:29 by nmeintje          #+#    #+#             */
/*   Updated: 2025/06/24 11:54:03 by nmeintje         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#define YELLOW "\033[33m"
#define RED "\033[31m"
#define DEFAULT "\033[0;0m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"
#define CYAN "\033[1;36m"
#define MAGENTA "\033[1;35m"


enum class LogLevel {
	INFO,
	WARNING,
	ERROR,
	DEBUG
};

class Logger {

	private:
		Logger() = delete;
		Logger(const Logger& copy) = delete;
		Logger &operator=(const Logger& other) = delete;		

	public:
		~Logger();
		
		static void	log(LogLevel level, const std::string& msg);
};
