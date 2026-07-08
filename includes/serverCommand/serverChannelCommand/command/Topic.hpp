#ifndef TOPIC_HPP
# define TOPIC_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Topic
{
public:
    Topic(ClientManager &, ChannelManager &);
    ~Topic();

    bool    handle(Client &, const Parser &);

private:
    ChannelManager  &_channels;

    Topic();
    Topic(const Topic &);
    Topic &operator=(const Topic &);
};

#endif
