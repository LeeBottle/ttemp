#include "server/Server.hpp"
#include "client/Client.hpp"
#include "server/Signal.hpp"

#include <iostream>
#include <string>
#include <unistd.h>


Server::Server(int port, const std::string &password)
    : _password(password),
      _clients(),
      _channels(),
      _listener(port),
      _poll(),
      _clientIO(),
      _message(_password, _clients, _channels),
      _event(),
      _signal()
{
}


Server::~Server()
{
}


bool    Server::run()
{
    std::vector<struct pollfd>  pollFds;

    if (!_signal.setup())
        return (false);

    if (!_listener.setup())
        return (false);

    while (!_signal.shouldStop())
    {
        _poll.build(pollFds, _listener, _clients);

        if (!_event.wait(pollFds))
            return (false);

        handlePoll(pollFds);
    }

    std::cout << "server shutting down" << std::endl;

    return (true);
}


void    Server::handlePoll(std::vector<struct pollfd> &pollFds)
{
    size_t index;
    int    fd;
    short  revents;

    index = pollFds.size();
    while (index > 0 && !_signal.shouldStop())
    {
        --index;
        fd = pollFds[index].fd;
        revents = pollFds[index].revents;
        if (revents == 0)
            continue ;

        if (fd == STDIN_FILENO)
        {
            if (revents & POLLIN)
                handleTerminal();
        }
        else if (fd == _listener.fd() && (revents & POLLIN))
            acceptClients();
        else if (fd != _listener.fd())
            handleClient(fd, revents);
    }
}


void    Server::handleClient(int clientFd, short revents)
{
    Client  *client;
    bool    hasReceiveData;

    hasReceiveData = false;
    if (revents & (POLLERR | POLLHUP | POLLNVAL))
    {
        _clientIO.remove(_clients, _channels, clientFd);
        return ;
    }

    if (revents & POLLIN)
    {
        if (!_clientIO.receive(_clients, _channels, clientFd))
            return ;

        hasReceiveData = true;
    }

    if (revents & POLLOUT)
        _clientIO.send(_clients, _channels, clientFd);

    client = _clients.findByFd(clientFd);
    if (hasReceiveData && client != NULL && !_message.process(*client))
    {
        _clientIO.send(_clients, _channels, clientFd);
        _clientIO.remove(_clients, _channels, clientFd);
    }
}


void    Server::handleTerminal()
{
    std::string line;

    if (!std::getline(std::cin, line))
        return ;

    if (line == "DIE")
        _signal.requestStop();
}


bool    Server::acceptClients()
{
    int clientFd;

    while (true)
    {
        if (!_listener.acceptClient(clientFd))
            return (false);

        if (clientFd == -1)
            return (true);

        _clients.add(clientFd);
        std::cout << "client connected with fd " << clientFd << std::endl;
    }
}
