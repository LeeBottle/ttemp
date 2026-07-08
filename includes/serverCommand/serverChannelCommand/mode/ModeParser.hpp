#ifndef MODEPARSER_HPP
# define MODEPARSER_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"
# include "serverCommand/serverChannelCommand/mode/ModeChange.hpp"

class Channel;
class ChannelManager;
class Client;
class ClientManager;
class Parser;

class ModeParser
{
public:
    ModeParser(ClientManager &, ChannelManager &);
    ~ModeParser();

    bool    collect(Client &, Channel &, const Parser &, ModeChange &);

private:
    ClientManager   &_clients;

    ModeParser();
    ModeParser(const ModeParser &);
    ModeParser &operator=(const ModeParser &);

    bool    collectLetter(Client &, Channel &,
                const std::vector<std::string> &, ModeChange &, char);
    bool    collectKey(Client &, Channel &, 
                const std::vector<std::string> &, ModeChange &);
    bool    collectLimit(Client &, Channel &,
                const std::vector<std::string> &, ModeChange &);
    bool    collectOperator(Client &, Channel &,
                const std::vector<std::string> &, ModeChange &);
    void    sendBanEnd(Client &, Channel &);
    bool    parseLimit(const std::string &, size_t &) const;
};

#endif
