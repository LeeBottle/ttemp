#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>

# include "channel/InviteList.hpp"
# include "channel/ModeState.hpp"
# include "channel/MemberList.hpp"
# include "channel/OperatorList.hpp"

class Client;

class Channel
{
public:
    Channel(const std::string &);
    ~Channel();

    const std::string   &name() const;
    
    MemberList    &members();
    OperatorList  &operators();
    InviteList    &invites();
    ModeState     &modes();

private:
    std::string     _name;
    MemberList      _members;
    OperatorList    _operators;
    InviteList      _invites;
    ModeState       _modes;

    Channel();
    Channel(const Channel &);
    Channel &operator=(const Channel &);
};

#endif
