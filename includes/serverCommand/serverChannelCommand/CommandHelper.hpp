#ifndef COMMAND_HELPER_HPP
# define COMMAND_HELPER_HPP

# include <string>

class Channel;
class Client;

bool    commandValidChannel(const std::string &name);
void    commandReply(Client &client, const std::string &message);
void    commandNamesReply(Client &client, Channel &channel);
void    commandTopicReply(Client &client, Channel &channel);
void    commandToAll(Channel &channel, const std::string &message);
void    commandToOthers(Channel &channel, Client &sender, const std::string &message);

std::string commandTarget(Client &client);
std::string commandMemberNames(Channel &channel);

#endif
