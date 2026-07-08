#ifndef CLIENTCOMMAND_HPP
# define CLIENTCOMMAND_HPP

# include <string>

class Client;
class ClientManager;
class Parser;

class ClientCommand
{
public:
    ClientCommand(const std::string &, ClientManager &);
    ~ClientCommand();

    bool    cap(Client &, const Parser &);
    bool    ping(Client &, const Parser &);
    bool    pong(Client &, const Parser &);
    bool    quit(Client &, const Parser &);
    bool    pass(Client &, const Parser &);
    bool    nick(Client &, const Parser &);
    bool    user(Client &, const Parser &);
    bool    privmsg(Client &, const Parser &);
    bool    unknown(Client &, const Parser &);

private:
    const std::string   &_password;
    ClientManager       &_clients;

    ClientCommand();
    ClientCommand(const ClientCommand &);
    ClientCommand &operator=(const ClientCommand &);

    const std::string   &target(Client &) const;
    void                reply(Client &, const std::string &) const;
    void                sendRegistrationIfReady(Client &, bool);
};

#endif
