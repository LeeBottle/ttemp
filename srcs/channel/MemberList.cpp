#include "channel/MemberList.hpp"
#include "client/Client.hpp"

#include <cstddef>


MemberList::MemberList() : _members()
{
}


MemberList::~MemberList()
{
}


const std::vector<Client *> &MemberList::all() const
{
    return (_members);
}


size_t  MemberList::count() const
{
    return (_members.size());
}


bool    MemberList::isEmpty() const
{
    return (_members.empty());
}


bool    MemberList::has(Client *member) const
{
    std::vector<Client *>::const_iterator   it;

    it = _members.begin();
    while (it != _members.end())
    {
        if (*it == member)
            return (true);
        ++it;
    }

    return (false);
}


void    MemberList::add(Client *member)
{
    if (member == NULL || has(member))
        return ;

    _members.push_back(member);
}


void    MemberList::remove(Client *member)
{
    std::vector<Client *>::iterator it;

    it = _members.begin();
    while (it != _members.end())
    {
        if (*it == member)
        {
            _members.erase(it);
            return ;
        }
        ++it;
    }
}
