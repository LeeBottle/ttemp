#ifndef JOIN_HPP
# define JOIN_HPP

# include "serverCommand/serverChannelCommand/CommandHelper.hpp"

class Channel;
class ChannelManager;
class Client;
class ClientManager;
class Parser;

class Join
{
public:
    Join(ClientManager &, ChannelManager &);
    ~Join();

    bool    handle(Client &, const Parser &);

private:
    enum JoinResult
    {
        JOIN_ALLOWED,
        JOIN_INVITE_ONLY,
        JOIN_BAD_KEY,
        JOIN_FULL
    };

    ChannelManager  &_channels;

    Join();
    Join(const Join &);
    Join &operator=(const Join &);

    JoinResult  checkPermission(Channel &, Client &, const std::string &) const;
    void        join(Channel &, Client &) const;
};

#endif
