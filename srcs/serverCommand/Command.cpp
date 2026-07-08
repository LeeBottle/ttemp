#include "serverCommand/Command.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"

#include <vector>


Command::Command(const std::string &password, ClientManager &clients,
    ChannelManager &channels)
    : _client(password, clients),
      _join(clients, channels),
      _part(clients, channels),
      _privmsg(clients, channels),
      _names(clients, channels),
      _who(clients, channels),
      _topic(clients, channels),
      _invite(clients, channels),
      _kick(clients, channels),
      _mode(clients, channels)
{
}


Command::~Command()
{
}


bool    Command::execute(Client &client, const Parser &message)
{
    switch (message.type())
    {
    case Parser::CAP:
        return (_client.cap(client, message));
    case Parser::PING:
        return (_client.ping(client, message));
    case Parser::PONG:
        return (_client.pong(client, message));
    case Parser::QUIT:
        return (_client.quit(client, message));
    case Parser::PASS:
        return (_client.pass(client, message));
    case Parser::NICK:
        return (_client.nick(client, message));
    case Parser::USER:
        return (_client.user(client, message));
    case Parser::JOIN:
        return (_join.handle(client, message));
    case Parser::PART:
        return (_part.handle(client, message));
    case Parser::PRIVMSG:
    {
        const std::vector<std::string>  &params = message.params();

        if (!params.empty() && !params[0].empty() && params[0][0] == '#')
            return (_privmsg.handle(client, message));

        return (_client.privmsg(client, message));
    }
    case Parser::NAMES:
        return (_names.handle(client, message));
    case Parser::WHO:
        return (_who.handle(client, message));
    case Parser::TOPIC:
        return (_topic.handle(client, message));
    case Parser::INVITE:
        return (_invite.handle(client, message));
    case Parser::KICK:
        return (_kick.handle(client, message));
    case Parser::MODE:
        return (_mode.handle(client, message));
    default:
        return (_client.unknown(client, message));
    }
}
