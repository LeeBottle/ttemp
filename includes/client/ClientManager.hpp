#ifndef CLIENTMANAGER_HPP
# define CLIENTMANAGER_HPP

# include <string>
# include <vector>

class Client;

class ClientManager
{
public:
    ClientManager();
    ~ClientManager();

    const std::vector<Client *> &clients() const;

    Client  *add(int);
    Client  *findByFd(int);
    Client  *findByNickname(const std::string &);

    void    removeByFd(int);
    void    clear();
    bool    isNicknameInUse(const std::string &, Client &) const;

private:
    std::vector<Client *>   _clients;

    ClientManager(const ClientManager &);
    ClientManager &operator=(const ClientManager &);
};

#endif
