#include "serverCommand/serverChannelCommand/mode/ModeChecker.hpp"
#include "channel/Channel.hpp"
#include "channel/ChannelManager.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"

#include <vector>


ModeChecker::ModeChecker(ClientManager &clients, ChannelManager &channels)
    : _channels(channels)
{
    (void)clients;
}


ModeChecker::~ModeChecker()
{
}


bool    ModeChecker::prepare(Client &client, const Parser &message,
    Channel *&channel)
{
    const std::vector<std::string>  &params = message.params();

    if (!client.isRegistered())
        commandReply(client, ":ircserv 451 " + commandTarget(client)
            + " :You have not registered\r\n");
    else if (params.empty())
        commandReply(client, ":ircserv 461 " + commandTarget(client)
            + " MODE :Not enough parameters\r\n");
    else if (user(client, message))
        return (true);
    else if ((channel = _channels.find(params[0])) == NULL)
        commandReply(client, ":ircserv 403 " + commandTarget(client)
            + " " + params[0] + " :No such channel\r\n");
    else if (banList(client, message, channel))
        return (true);
    else if (params.size() == 1)
        commandReply(client, ":ircserv 324 " + commandTarget(client)
            + " " + channel->name() + " " + channel->modes().modeString()
            + channel->modes().modeParameters() + "\r\n");
    else if (!channel->members().has(&client))
        commandReply(client, ":ircserv 442 " + commandTarget(client)
            + " " + params[0] + " :You're not on that channel\r\n");
    else if (!channel->operators().has(&client))
        commandReply(client, ":ircserv 482 " + commandTarget(client)
            + " " + params[0] + " :You're not channel operator\r\n");
    else
        return (false);

    return (true);
}


bool    ModeChecker::user(Client &client, const Parser &message)
{
    const std::vector<std::string>  &params = message.params();

    if (commandValidChannel(params[0]))
        return (false);

    if (params[0] != client.nickname())
    {
        commandReply(client, ":ircserv 502 " + commandTarget(client)
            + " :Cant change mode for other users\r\n");

        return (true);
    }

    if (params.size() == 1)
        commandReply(client, ":ircserv 221 " + commandTarget(client) 
            + " +i\r\n");

    return (true);
}


bool    ModeChecker::banList(Client &client, const Parser &message,
    Channel *&channel)
{
    const std::vector<std::string>  &params = message.params();
    const std::string               &modeString = params[1];
    size_t                          index;

    if (params.size() != 2)
        return (false);

    index = 0;
    while (index < modeString.size())
    {
        if (modeString[index] != '+' && modeString[index] != '-'
            && modeString[index] != 'b')
            return (false);
        ++index;
    }

    if (modeString.find('b') == std::string::npos)
        return (false);

    commandReply(client, ":ircserv 368 " + commandTarget(client)
        + " " + channel->name() + " :End of Channel Ban List\r\n");

    return (true);
}
