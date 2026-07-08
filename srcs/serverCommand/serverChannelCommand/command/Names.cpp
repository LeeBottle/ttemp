#include "serverCommand/serverChannelCommand/command/Names.hpp"
#include "channel/Channel.hpp"
#include "channel/ChannelManager.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"

#include <vector>


Names::Names(ClientManager &clients, ChannelManager &channels)
    : _channels(channels)
{
    (void)clients;
}


Names::~Names()
{
}


bool    Names::handle(Client &client, const Parser &message)
{
    const std::vector<std::string>  &params = message.params();
    Channel                         *channel;

    if (!client.isRegistered())
        commandReply(client, ":ircserv 451 " + commandTarget(client)
            + " :You have not registered\r\n");
    else if (params.empty())
        commandReply(client, ":ircserv 461 " + commandTarget(client)
            + " NAMES :Not enough parameters\r\n");
    else if (!commandValidChannel(params[0]))
        commandReply(client, ":ircserv 403 " + commandTarget(client)
            + " " + params[0] + " :No such channel\r\n");
    else
    {
        channel = _channels.find(params[0]);
        if (channel == NULL)
            commandReply(client, ":ircserv 403 " + commandTarget(client)
                + " " + params[0] + " :No such channel\r\n");
        else
            commandNamesReply(client, *channel);
    }

    return (true);
}
