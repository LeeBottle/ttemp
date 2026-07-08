#ifndef INVITE_HPP
# define INVITE_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Invite
{
public:
    Invite(ClientManager &, ChannelManager &);
    ~Invite();

    bool    handle(Client &, const Parser &);

private:
    ClientManager   &_clients;
    ChannelManager  &_channels;

    Invite();
    Invite(const Invite &);
    Invite &operator=(const Invite &);
};

#endif
