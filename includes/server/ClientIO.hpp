#ifndef CLIENTIO_HPP
# define CLIENTIO_HPP

class ChannelManager;
class ClientManager;

class ClientIO
{
public:
    ClientIO();
    ~ClientIO();

    bool    receive(ClientManager &, ChannelManager &, int);
    void    remove(ClientManager &, ChannelManager &, int);
    void    send(ClientManager &, ChannelManager &, int);

private:
    ClientIO(const ClientIO &);
    ClientIO &operator=(const ClientIO &);
};

#endif
