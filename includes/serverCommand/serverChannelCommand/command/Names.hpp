#ifndef NAMES_HPP
# define NAMES_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Names
{
public:
    Names(ClientManager &, ChannelManager &);
    ~Names();

    bool    handle(Client &, const Parser &);

private:
    ChannelManager  &_channels;

    Names();
    Names(const Names &);
    Names &operator=(const Names &);
};

#endif
