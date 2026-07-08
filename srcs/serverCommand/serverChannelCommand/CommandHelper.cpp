#include "serverCommand/serverChannelCommand/CommandHelper.hpp"
#include "channel/Channel.hpp"
#include "client/Client.hpp"

#include <vector>


bool    commandValidChannel(const std::string &name)
{
    size_t  index;

    if (name.size() < 2 || name[0] != '#')
        return (false);

    index = 0;
    while (index < name.size())
    {
        if (name[index] == ' ' || name[index] == ',' || name[index] == '\r'
            || name[index] == '\n' || name[index] == '\0')
            return (false);
        ++index;
    }

    return (true);
}


void    commandReply(Client &client, const std::string &message)
{
    client.sendBuffer().append(message);
}


void    commandNamesReply(Client &client, Channel &channel)
{
    commandReply(client, ":ircserv 353 " + commandTarget(client)
        + " = " + channel.name() + " :" + commandMemberNames(channel)
        + "\r\n");
    commandReply(client, ":ircserv 366 " + commandTarget(client)
        + " " + channel.name() + " :End of /NAMES list\r\n");
}


void    commandTopicReply(Client &client, Channel &channel)
{
    if (channel.modes().topic().empty())
        commandReply(client, ":ircserv 331 " + commandTarget(client)
            + " " + channel.name() + " :No topic is set\r\n");
    else
        commandReply(client, ":ircserv 332 " + commandTarget(client)
            + " " + channel.name() + " :" + channel.modes().topic()
            + "\r\n");
}


void    commandToAll(Channel &channel, const std::string &message)
{
    std::vector<Client *>::const_iterator   it;
    const std::vector<Client *>             &members = channel.members().all();

    it = members.begin();
    while (it != members.end())
    {
        (*it)->sendBuffer().append(message);
        ++it;
    }
}


void    commandToOthers(Channel &channel, Client &sender,
    const std::string &message)
{
    std::vector<Client *>::const_iterator   it;
    const std::vector<Client *>             &members = channel.members().all();

    it = members.begin();
    while (it != members.end())
    {
        if (*it != &sender)
            (*it)->sendBuffer().append(message);
        ++it;
    }
}


std::string commandTarget(Client &client)
{
    if (client.hasNickname())
        return (client.nickname());

    return ("*");
}


std::string commandMemberNames(Channel &channel)
{
    std::vector<Client *>::const_iterator   it;
    const std::vector<Client *>             &members = channel.members().all();
    std::string                             names;

    it = members.begin();
    while (it != members.end())
    {
        if ((*it)->hasNickname())
        {
            if (!names.empty())
                names += " ";

            if (channel.operators().has(*it))
                names += "@";

            names += (*it)->nickname();
        }
        ++it;
    }

    return (names);
}
