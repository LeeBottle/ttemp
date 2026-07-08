#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>

# include "client/ReceiveBuffer.hpp"
# include "client/SendBuffer.hpp"

class Client
{
public:
    Client(int);
    ~Client();

    const std::string   &nickname() const;
    const std::string   &username() const;
    const std::string   &realname() const;
    ReceiveBuffer       &receiveBuffer();
    SendBuffer          &sendBuffer();
    std::string         prefix() const;
    
    int     fd() const;
    bool    hasPassword() const;
    bool    hasNickname() const;
    bool    hasUser() const;
    bool    isRegistered() const;
    void    acceptPassword();
    void    setNickname(const std::string &);
    void    setUser(const std::string &, const std::string &);

private:
    int             _fd;
    bool            _hasPassword;
    bool            _registered;
    std::string     _nickname;
    std::string     _username;
    std::string     _realname;
    ReceiveBuffer   _receive;
    SendBuffer      _send;

    void    updateRegistration();

    Client();
    Client(const Client &);
    Client &operator=(const Client &);
};

#endif
