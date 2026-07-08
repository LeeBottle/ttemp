#include "serverCommand/serverChannelCommand/command/Topic.hpp"
#include "channel/Channel.hpp"
#include "channel/ChannelManager.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"

#include <vector>


Topic::Topic(ClientManager &clients, ChannelManager &channels)
    : _channels(channels)
{
    (void)clients;
}


Topic::~Topic()
{
}


bool    Topic::handle(Client &client, const Parser &message)
{
    const std::vector<std::string>  &params = message.params();
    Channel                         *channel;

    if (!client.isRegistered())
        commandReply(client, ":ircserv 451 " + commandTarget(client)
            + " :You have not registered\r\n");
    else if (params.empty())
        commandReply(client, ":ircserv 461 " + commandTarget(client)
            + " TOPIC :Not enough parameters\r\n");
    else
    {
        channel = _channels.find(params[0]);

        if (channel == NULL)
            commandReply(client, ":ircserv 403 " + commandTarget(client)
                + " " + params[0] + " :No such channel\r\n");
        else if (params.size() == 1)
            commandTopicReply(client, *channel);
        else if (!channel->members().has(&client))
            commandReply(client, ":ircserv 442 " + commandTarget(client)
                + " " + params[0] + " :You're not on that channel\r\n");
        else if (channel->modes().topicRestricted()
            && !channel->operators().has(&client))
            commandReply(client, ":ircserv 482 " + commandTarget(client)
                + " " + params[0] + " :You're not channel operator\r\n");
        else
        {
            channel->modes().setTopic(params[1]);
            commandToAll(*channel, ":" + client.prefix()
                + " TOPIC " + channel->name() + " :" + params[1] + "\r\n");
        }
    }

    return (true);
}
