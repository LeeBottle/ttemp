#ifndef WHO_HPP
# define WHO_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Who
{
public:
    Who(ClientManager &, ChannelManager &);
    ~Who();

    bool    handle(Client &, const Parser &);

private:
    ChannelManager  &_channels;

    Who();
    Who(const Who &);
    Who &operator=(const Who &);
};

#endif
