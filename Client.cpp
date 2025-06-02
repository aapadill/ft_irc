
#include "Client.hpp"

Client::Client(std::string nick, int sock) : _nickname(std::move(nick)), _socket(sock) {}

std::string Client::getNickname() const
{ 
    return _nickname;
}

void Client::sendMessage(const std::string& message) 
{
        // send message to socket_
        std::cout << "To [" << _nickname << "]: " << message << "\n";
}