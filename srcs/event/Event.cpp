#include "event/Event.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>


Event::Event()
{
}

Event::~Event()
{
}


bool    Event::wait(std::vector<struct pollfd> &pollFds)
{
    int readyCount;

    readyCount = ::poll(&pollFds[0], pollFds.size(), -1);
    if (readyCount == -1)
    {
        if (errno == EINTR)
            return (true);

        return (reportSystemError("poll"));
    }

    return (true);
}


bool    Event::reportSystemError(const char *functionName)
{
    std::cerr << functionName << ": " << std::strerror(errno) << std::endl;
    return (false);
}
