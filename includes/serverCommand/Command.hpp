#ifndef COMMAND_HPP
# define COMMAND_HPP

# include <string>

# include "serverCommand/serverChannelCommand/command/Invite.hpp"
# include "serverCommand/serverChannelCommand/command/Join.hpp"
# include "serverCommand/serverChannelCommand/command/Kick.hpp"
# include "serverCommand/serverChannelCommand/command/Names.hpp"
# include "serverCommand/serverChannelCommand/command/Part.hpp"
# include "serverCommand/serverChannelCommand/command/Privmsg.hpp"
# include "serverCommand/serverChannelCommand/command/Topic.hpp"
# include "serverCommand/serverChannelCommand/command/Who.hpp"
# include "serverCommand/serverChannelCommand/mode/Mode.hpp"
# include "serverCommand/serverClientCommand/ClientCommand.hpp"

class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Command
{
public:
    Command(const std::string &, ClientManager &, ChannelManager &);
    ~Command();

    bool    execute(Client &, const Parser &);

private:
    ClientCommand   _client;
    Join            _join;
    Part            _part;
    Privmsg         _privmsg;
    Names           _names;
    Who             _who;
    Topic           _topic;
    Invite          _invite;
    Kick            _kick;
    Mode            _mode;

    Command();
    Command(const Command &);
    Command &operator=(const Command &);
};

#endif
