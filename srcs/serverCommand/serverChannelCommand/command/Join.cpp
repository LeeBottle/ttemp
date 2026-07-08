#include "serverCommand/serverChannelCommand/command/Join.hpp"
#include "channel/Channel.hpp"
#include "channel/ChannelManager.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"

#include <vector>


Join::Join(ClientManager &clients, ChannelManager &channels) 
    : _channels(channels)
{
    (void)clients;
}


Join::~Join()
{
}


bool    Join::handle(Client &client, const Parser &message)
{
    const std::vector<std::string>  &params = message.params();
    Channel                         *channel;
    JoinResult                      result;
    std::string                     key;

    if (!client.isRegistered())
    {
        commandReply(client, ":ircserv 451 " + commandTarget(client)
            + " :You have not registered\r\n");
        return (true);
    }

    if (params.empty())
    {
        commandReply(client, ":ircserv 461 " + commandTarget(client)
            + " JOIN :Not enough parameters\r\n");
        return (true);
    }

    if (!commandValidChannel(params[0]))
    {
        commandReply(client, ":ircserv 403 " + commandTarget(client)
            + " " + params[0] + " :No such channel\r\n");
        return (true);
    }

    if (params.size() > 1)
        key = params[1];

    channel = _channels.findOrCreate(params[0]);
    result = checkPermission(*channel, client, key);

    if (result == JOIN_INVITE_ONLY)
        commandReply(client, ":ircserv 473 " + commandTarget(client)
            + " " + params[0] + " :Cannot join channel (+i)\r\n");
    else if (result == JOIN_BAD_KEY)
        commandReply(client, ":ircserv 475 " + commandTarget(client)
            + " " + params[0] + " :Cannot join channel (+k)\r\n");
    else if (result == JOIN_FULL)
        commandReply(client, ":ircserv 471 " + commandTarget(client)
            + " " + params[0] + " :Cannot join channel (+l)\r\n");
    else
    {
        join(*channel, client);
        commandToAll(*channel, ":" + client.prefix() + " JOIN "
            + channel->name() + "\r\n");

        if (!channel->modes().topic().empty())
            commandTopicReply(client, *channel);

        commandNamesReply(client, *channel);
    }

    return (true);
}


Join::JoinResult    Join::checkPermission(Channel &channel, Client &client,
    const std::string &key) const
{
    if (channel.modes().inviteOnly()
        && !channel.invites().has(&client)
        && !channel.members().has(&client))
        return (JOIN_INVITE_ONLY);

    if (channel.modes().hasKey()
        && key != channel.modes().key()
        && !channel.members().has(&client))
        return (JOIN_BAD_KEY);

    if (channel.modes().hasLimit()
        && channel.members().count() >= channel.modes().limit()
        && !channel.members().has(&client))
        return (JOIN_FULL);

    return (JOIN_ALLOWED);
}


void    Join::join(Channel &channel, Client &client) const
{
    if (channel.members().has(&client))
        return ;

    channel.members().add(&client);
    if (channel.members().count() == 1)
        channel.operators().add(&client);

    channel.invites().remove(&client);
}
