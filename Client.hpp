#pragma once

#include <string>
#include <iostream>

class Client {

    private:
    
        std::string _nickname;
        int         _socket;

    public:
    
        Client(std::string nick, int sock);
        std::string getNickname() const;
        void sendMessage(const std::string& message);
};
