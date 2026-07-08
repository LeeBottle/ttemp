#ifndef KICK_HPP
# define KICK_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Kick
{
public:
    Kick(ClientManager &, ChannelManager &);
    ~Kick();

    bool    handle(Client &, const Parser &);

private:
    ClientManager   &_clients;
    ChannelManager  &_channels;

    Kick();
    Kick(const Kick &);
    Kick &operator=(const Kick &);
};

#endif
