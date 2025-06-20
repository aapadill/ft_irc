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
        int         _socket;

    public:
    
        User(std::string nick, int sock);
        std::string getNickname() const;
        void sendMessage(const std::string& message);
};
