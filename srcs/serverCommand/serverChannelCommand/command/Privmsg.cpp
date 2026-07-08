#include "serverCommand/serverChannelCommand/command/Privmsg.hpp"
#include "channel/Channel.hpp"
#include "channel/ChannelManager.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"

#include <vector>


Privmsg::Privmsg(ClientManager &clients, ChannelManager &channels)
    : _channels(channels)
{
    (void)clients;
}


Privmsg::~Privmsg()
{
}


bool    Privmsg::handle(Client &client, const Parser &message)
{
    const std::vector<std::string>  &params = message.params();

    if (!client.isRegistered())
    {
        commandReply(client, ":ircserv 451 " + commandTarget(client)
            + " :You have not registered\r\n");
        return (true);
    }

    if (params.empty())
    {
        commandReply(client, ":ircserv 411 " + commandTarget(client)
            + " :No recipient given (PRIVMSG)\r\n");
        return (true);
    }

    if (params.size() < 2 || params[1].empty())
    {
        commandReply(client, ":ircserv 412 " + commandTarget(client)
            + " :No text to send\r\n");
        return (true);
    }

    sendToChannel(client, params[0], params[1]);

    return (true);
}


void    Privmsg::sendToChannel(Client &client, const std::string &target,
    const std::string &text)
{
    Channel *channel;

    channel = _channels.find(target);

    if (channel == NULL)
        commandReply(client, ":ircserv 403 " + commandTarget(client)
            + " " + target + " :No such channel\r\n");
    else if (!channel->members().has(&client))
        commandReply(client, ":ircserv 404 " + commandTarget(client)
            + " " + target + " :Cannot send to channel\r\n");
    else
        commandToOthers(*channel, client, ":" + client.prefix()
            + " PRIVMSG " + target + " :" + text + "\r\n");
}
