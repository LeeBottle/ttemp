#ifndef MODE_HPP
# define MODE_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"
# include "serverCommand/serverChannelCommand/mode/ModeApplier.hpp"
# include "serverCommand/serverChannelCommand/mode/ModeChecker.hpp"
# include "serverCommand/serverChannelCommand/mode/ModeParser.hpp"
# include "serverCommand/serverChannelCommand/mode/ModeChange.hpp"

class Channel;
class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Mode
{
public:
    Mode(ClientManager &, ChannelManager &);
    ~Mode();

    bool    handle(Client &, const Parser &);

private:
    ModeChecker  _checker;
    ModeParser   _parser;
    ModeApplier  _applier;

    Mode();
    Mode(const Mode &);
    Mode &operator=(const Mode &);

    void    broadcast(Client &, Channel &, const ModeChange &) const;
};

#endif
