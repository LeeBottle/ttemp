#ifndef INVITELIST_HPP
# define INVITELIST_HPP

# include <vector>

class Client;

class InviteList
{
public:
    InviteList();
    ~InviteList();

    bool    has(Client *) const;
    void    add(Client *);
    void    remove(Client *);

private:
    std::vector<Client *>   _invites;

    InviteList(const InviteList &);
    InviteList &operator=(const InviteList &);
};

#endif
