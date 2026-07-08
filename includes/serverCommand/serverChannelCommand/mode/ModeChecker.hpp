#ifndef MODECHECKER_HPP
# define MODECHECKER_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class Channel;
class ChannelManager;
class Client;
class ClientManager;
class Parser;

class ModeChecker
{
public:
    ModeChecker(ClientManager &, ChannelManager &);
    ~ModeChecker();

    bool    prepare(Client &, const Parser &, Channel *&);

private:
    ChannelManager  &_channels;

    ModeChecker();
    ModeChecker(const ModeChecker &);
    ModeChecker &operator=(const ModeChecker &);

    bool    user(Client &, const Parser &);
    bool    banList(Client &, const Parser &, Channel *&);
};

#endif
