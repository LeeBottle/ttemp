#ifndef PRIVMSG_HPP
# define PRIVMSG_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Privmsg
{
public:
    Privmsg(ClientManager &, ChannelManager &);
    ~Privmsg();

    bool    handle(Client &, const Parser &);

private:
    ChannelManager  &_channels;

    Privmsg();
    Privmsg(const Privmsg &);
    Privmsg &operator=(const Privmsg &);

    void    sendToChannel(Client &, const std::string &, const std::string &);
};

#endif
