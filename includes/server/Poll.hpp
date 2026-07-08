#ifndef POLL_HPP
# define POLL_HPP

# include <poll.h>
# include <vector>

class ClientManager;
class Listener;

class Poll
{
public:
    Poll();
    ~Poll();

    void    build(std::vector<struct pollfd> &, Listener &, ClientManager &) const;

private:
    Poll(const Poll &);
    Poll &operator=(const Poll &);

    void    appendClients(std::vector<struct pollfd> &, ClientManager &) const;
    void    appendListener(std::vector<struct pollfd> &, Listener &) const;
    void    appendTerminal(std::vector<struct pollfd> &) const;
};

#endif
