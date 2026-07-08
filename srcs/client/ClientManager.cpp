#include "client/ClientManager.hpp"
#include "client/Client.hpp"


ClientManager::ClientManager() : _clients()
{
}


ClientManager::~ClientManager()
{
    clear();
}


Client  *ClientManager::add(int clientFd)
{
    Client  *client;

    client = new Client(clientFd);
    _clients.push_back(client);

    return (client);
}


Client  *ClientManager::findByFd(int clientFd)
{
    std::vector<Client *>::iterator it;

    it = _clients.begin();
    while (it != _clients.end())
    {
        if ((*it)->fd() == clientFd)
            return (*it);
        ++it;
    }

    return (NULL);
}


Client  *ClientManager::findByNickname(const std::string &nickname)
{
    std::vector<Client *>::iterator it;

    it = _clients.begin();
    while (it != _clients.end())
    {
        if ((*it)->nickname() == nickname)
            return (*it);
        ++it;
    }

    return (NULL);
}


const std::vector<Client *> &ClientManager::clients() const
{
    return (_clients);
}


void    ClientManager::removeByFd(int clientFd)
{
    std::vector<Client *>::iterator it;

    it = _clients.begin();
    while (it != _clients.end())
    {
        if ((*it)->fd() == clientFd)
        {
            delete *it;
            _clients.erase(it);
            return ;
        }
        ++it;
    }
}


void    ClientManager::clear()
{
    std::vector<Client *>::iterator it;

    it = _clients.begin();
    while (it != _clients.end())
    {
        delete *it;
        ++it;
    }
    _clients.clear();
}


bool    ClientManager::isNicknameInUse(const std::string &nickname,
    Client &owner) const
{
    std::vector<Client *>::const_iterator   it;

    it = _clients.begin();
    while (it != _clients.end())
    {
        if (*it != &owner && (*it)->nickname() == nickname)
            return (true);
        ++it;
    }

    return (false);
}
