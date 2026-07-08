#include "serverCommand/serverChannelCommand/command/Who.hpp"
#include "channel/Channel.hpp"
#include "channel/ChannelManager.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"

#include <vector>


Who::Who(ClientManager &clients, ChannelManager &channels)
    : _channels(channels)
{
    (void)clients;
}


Who::~Who()
{
}


bool    Who::handle(Client &client, const Parser &message)
{
    const std::vector<std::string>          &params = message.params();
    Channel                                 *channel;
    std::vector<Client *>::const_iterator   it;
    std::string                             realname;

    if (!client.isRegistered())
        commandReply(client, ":ircserv 451 " + commandTarget(client)
            + " :You have not registered\r\n");
    else if (params.empty())
        commandReply(client, ":ircserv 461 " + commandTarget(client)
            + " WHO :Not enough parameters\r\n");
    else
    {
        channel = _channels.find(params[0]);
        if (channel != NULL)
        {
            const std::vector<Client *> &members = channel->members().all();

            it = members.begin();
            while (it != members.end())
            {
                realname = (*it)->realname();
                if (realname.empty())
                    realname = (*it)->nickname();

                commandReply(client, ":ircserv 352 " + commandTarget(client)
                    + " " + channel->name() + " " + (*it)->username()
                    + " localhost ircserv " + (*it)->nickname()
                    + " H :0 " + realname + "\r\n");
                ++it;
            }
        }

        commandReply(client, ":ircserv 315 " + commandTarget(client)
            + " " + params[0] + " :End of WHO list\r\n");
    }

    return (true);
}
