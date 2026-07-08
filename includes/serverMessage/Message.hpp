#ifndef MESSAGE_HPP
# define MESSAGE_HPP

# include <string>

# include "serverCommand/Command.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Message
{
public:
    Message(const std::string &, ClientManager &, ChannelManager &);
    ~Message();

    bool    process(Client &);

private:
    Command   _command;

    Message();
    Message(const Message &);
    Message &operator=(const Message &);

    bool    handle(Client &, const std::string &);
};

#endif
