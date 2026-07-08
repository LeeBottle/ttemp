#include "channel/OperatorList.hpp"

#include <cstddef>


OperatorList::OperatorList() : _operators()
{
}


OperatorList::~OperatorList()
{
}


bool    OperatorList::has(Client *client) const
{
    std::vector<Client *>::const_iterator   it;

    it = _operators.begin();
    while (it != _operators.end())
    {
        if (*it == client)
            return (true);
        ++it;
    }

    return (false);
}


bool    OperatorList::add(Client *client)
{
    if (client == NULL || has(client))
        return (false);

    _operators.push_back(client);

    return (true);
}


bool    OperatorList::remove(Client *client)
{
    std::vector<Client *>::iterator it;

    it = _operators.begin();
    while (it != _operators.end())
    {
        if (*it == client)
        {
            _operators.erase(it);
            return (true);
        }
        ++it;
    }

    return (false);
}
