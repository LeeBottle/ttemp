#include "serverCommand/serverChannelCommand/command/Invite.hpp"
#include "channel/Channel.hpp"
#include "channel/ChannelManager.hpp"
#include "client/Client.hpp"
#include "client/ClientManager.hpp"
#include "parser/Parser.hpp"

#include <vector>


Invite::Invite(ClientManager &clients, ChannelManager &channels)
    : _clients(clients),
      _channels(channels)
{
}


Invite::~Invite()
{
}


bool    Invite::handle(Client &client, const Parser &message)
{
    const std::vector<std::string>  &params = message.params();
    Client                          *targetClient;
    Channel                         *channel;

    if (!client.isRegistered())
        commandReply(client, ":ircserv 451 " + commandTarget(client)
            + " :You have not registered\r\n");
    else if (params.size() < 2)
        commandReply(client, ":ircserv 461 " + commandTarget(client)
            + " INVITE :Not enough parameters\r\n");
    else
    {
        targetClient = _clients.findByNickname(params[0]);
        channel = _channels.find(params[1]);
        if (targetClient == NULL)
            commandReply(client, ":ircserv 401 " + commandTarget(client)
                + " " + params[0] + " :No such nick/channel\r\n");
        else if (channel == NULL)
            commandReply(client, ":ircserv 403 " + commandTarget(client)
                + " " + params[1] + " :No such channel\r\n");
        else if (!channel->members().has(&client))
            commandReply(client, ":ircserv 442 " + commandTarget(client)
                + " " + params[1] + " :You're not on that channel\r\n");
        else if (!channel->operators().has(&client))
            commandReply(client, ":ircserv 482 " + commandTarget(client)
                + " " + params[1] + " :You're not channel operator\r\n");
        else if (channel->members().has(targetClient))
            commandReply(client, ":ircserv 443 " + commandTarget(client)
                + " " + targetClient->nickname() + " " + params[1]
                + " :is already on channel\r\n");
        else
        {
            channel->invites().add(targetClient);
            commandReply(client, ":ircserv 341 " + commandTarget(client)
                + " " + targetClient->nickname() + " " + params[1]
                + "\r\n");
            targetClient->sendBuffer().append(":" + client.prefix()
                + " INVITE " + targetClient->nickname() + " :"
                + params[1] + "\r\n");
        }
    }

    return (true);
}
