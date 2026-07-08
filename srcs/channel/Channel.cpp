#include "channel/Channel.hpp"


Channel::Channel(const std::string &name)
    : _name(name),
      _members(),
      _operators(),
      _invites(),
      _modes()
{
}


Channel::~Channel()
{
}


const std::string   &Channel::name() const
{
    return (_name);
}


MemberList   &Channel::members()
{
    return (_members);
}


OperatorList &Channel::operators()
{
    return (_operators);
}


InviteList   &Channel::invites()
{
    return (_invites);
}


ModeState    &Channel::modes()
{
    return (_modes);
}
