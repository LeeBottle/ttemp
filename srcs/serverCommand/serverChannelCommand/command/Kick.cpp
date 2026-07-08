#include "serverCommand/serverChannelCommand/command/Kick.hpp"
#include "channel/Channel.hpp"
#include "channel/ChannelManager.hpp"
#include "client/Client.hpp"
#include "client/ClientManager.hpp"
#include "parser/Parser.hpp"

#include <vector>


Kick::Kick(ClientManager &clients, ChannelManager &channels)
    : _clients(clients),
      _channels(channels)
{
}


Kick::~Kick()
{
}


bool    Kick::handle(Client &client, const Parser &message)
{
    const std::vector<std::string>  &params = message.params();
    Channel                         *channel;
    Client                          *targetClient;
    std::string                     comment;

    if (!client.isRegistered())
        commandReply(client, ":ircserv 451 " + commandTarget(client)
            + " :You have not registered\r\n");
    else if (params.size() < 2)
        commandReply(client, ":ircserv 461 " + commandTarget(client)
            + " KICK :Not enough parameters\r\n");
    else
    {
        channel = _channels.find(params[0]);
        targetClient = _clients.findByNickname(params[1]);

        if (channel == NULL)
            commandReply(client, ":ircserv 403 " + commandTarget(client)
                + " " + params[0] + " :No such channel\r\n");
        else if (!channel->members().has(&client))
            commandReply(client, ":ircserv 442 " + commandTarget(client)
                + " " + params[0] + " :You're not on that channel\r\n");
        else if (!channel->operators().has(&client))
            commandReply(client, ":ircserv 482 " + commandTarget(client)
                + " " + params[0] + " :You're not channel operator\r\n");
        else if (targetClient == NULL || !channel->members().has(targetClient))
            commandReply(client, ":ircserv 441 " + commandTarget(client)
                + " " + params[1] + " " + params[0]
                + " :They aren't on that channel\r\n");
        else
        {
            comment = client.nickname();

            if (params.size() > 2 && !params[2].empty())
                comment = params[2];

            commandToAll(*channel, ":" + client.prefix() + " KICK "
                + channel->name() + " " + targetClient->nickname()
                + " :" + comment + "\r\n");
            channel->members().remove(targetClient);
            channel->operators().remove(targetClient);
            channel->invites().remove(targetClient);
            _channels.removeEmpty(channel);
        }
    }

    return (true);
}
