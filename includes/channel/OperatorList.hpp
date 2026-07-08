#ifndef OPERATORLIST_HPP
# define OPERATORLIST_HPP

# include <vector>

class Client;

class OperatorList
{
public:
    OperatorList();
    ~OperatorList();

    bool    has(Client *) const;
    bool    add(Client *);
    bool    remove(Client *);

private:
    std::vector<Client *>   _operators;

    OperatorList(const OperatorList &);
    OperatorList &operator=(const OperatorList &);
};

#endif
