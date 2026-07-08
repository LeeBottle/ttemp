#ifndef EVENT_HPP
# define EVENT_HPP

# include <poll.h>
# include <vector>

class Event
{
public:
    Event();
    ~Event();

    bool    wait(std::vector<struct pollfd> &);

private:
    Event(const Event &);
    Event &operator=(const Event &);

    bool    reportSystemError(const char *);
};

#endif
