#ifndef SERVER_HPP
# define SERVER_HPP

# include <poll.h>
# include <string>
# include <vector>

# include "channel/ChannelManager.hpp"
# include "client/ClientManager.hpp"
# include "event/Event.hpp"
# include "server/Listener.hpp"
# include "server/ClientIO.hpp"
# include "server/Poll.hpp"
# include "server/Signal.hpp"
# include "serverMessage/Message.hpp"

class Server
{
public:
    Server(int, const std::string &);
    ~Server();

    bool    run();

private:
    std::string     _password;
    ClientManager   _clients;
    ChannelManager  _channels;
    Listener        _listener;
    Poll            _poll;
    ClientIO        _clientIO;
    Message         _message;
    Event           _event;
    Signal          _signal;

    Server();
    Server(const Server &);
    Server &operator=(const Server &);

    bool    acceptClients();
    void    handleClient(int, short);
    void    handlePoll(std::vector<struct pollfd> &);
    void    handleTerminal();
};

#endif
