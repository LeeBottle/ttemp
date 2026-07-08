#ifndef PART_HPP
# define PART_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Part
{
public:
    Part(ClientManager &, ChannelManager &);
    ~Part();

    bool    handle(Client &, const Parser &);

private:
    ChannelManager  &_channels;

    Part();
    Part(const Part &);
    Part &operator=(const Part &);
};

#endif
