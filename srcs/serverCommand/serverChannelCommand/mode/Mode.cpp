#include "serverCommand/serverChannelCommand/mode/Mode.hpp"
#include "channel/Channel.hpp"
#include "client/Client.hpp"
#include "client/ClientManager.hpp"
#include "parser/Parser.hpp"


Mode::Mode(ClientManager &clients, ChannelManager &channels)
    : _checker(clients, channels),
      _parser(clients, channels),
      _applier()
{
}


Mode::~Mode()
{
}


bool    Mode::handle(Client &client, const Parser &message)
{
    Channel                 *channel;
    ModeChange              change;

    channel = NULL;
    if (_checker.prepare(client, message, channel))
        return (true);

    if (!_parser.collect(client, *channel, message, change))
        return (true);

    _applier.apply(*channel, change);
    broadcast(client, *channel, change);

    return (true);
}


void    Mode::broadcast(Client &client, Channel &channel,
    const ModeChange &change) const
{
    if (!change.changes.empty())
        commandToAll(channel, ":" + client.prefix() + " MODE "
            + channel.name() + " " + change.changes + change.params + "\r\n");
}
