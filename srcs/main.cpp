#include "server/Server.hpp"

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>


static int  convertPort(const std::string &strPort)
{
    int     port;
    size_t  i;

    if (strPort.empty())
        return (-1);

    port = 0;
    i = 0;
    while (i < strPort.size())
    {
        if (strPort[i] < '0' || strPort[i] > '9')
            return (-1);

        port = port * 10 + (strPort[i] - '0');
        if (port > 65535)
            return (-1);

        ++i;
    }

    if (port == 0)
        return (-1);

    return (port);
}


static bool isValidPassword(const std::string &password)
{
    if (password.empty() || password.size() > 504)
        return (false);

    if (password.find('\0') != std::string::npos)
        return (false);

    if (password.find('\r') != std::string::npos)
        return (false);

    if (password.find('\n') != std::string::npos)
        return (false);

    return (true);
}


int main(int argc, char **argv)
{
    int         port;
    std::string password;

    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return (EXIT_FAILURE);
    }

    port = convertPort(argv[1]);
    if (port == -1)
    {
        std::cerr << "Error: invalid port" << std::endl;
        return (EXIT_FAILURE);
    }

    password = argv[2];
    if (!isValidPassword(password))
    {
        std::cerr << "Error: invalid password" << std::endl;
        return (EXIT_FAILURE);
    }

    Server server(port, password);

    if (!server.run())
        return (EXIT_FAILURE);

    return (EXIT_SUCCESS);
}
