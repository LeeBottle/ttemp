#include "channel/InviteList.hpp"

#include <cstddef>


InviteList::InviteList() : _invites()
{
}


InviteList::~InviteList()
{
}


bool    InviteList::has(Client *client) const
{
    std::vector<Client *>::const_iterator   it;

    it = _invites.begin();
    while (it != _invites.end())
    {
        if (*it == client)
            return (true);
        ++it;
    }

    return (false);
}


void    InviteList::add(Client *client)
{
    if (client != NULL && !has(client))
        _invites.push_back(client);
}


void    InviteList::remove(Client *client)
{
    std::vector<Client *>::iterator it;

    it = _invites.begin();
    while (it != _invites.end())
    {
        if (*it == client)
        {
            _invites.erase(it);
            return ;
        }
        ++it;
    }
}
