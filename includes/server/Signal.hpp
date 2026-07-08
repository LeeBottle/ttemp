#ifndef SIGNAL_HPP
# define SIGNAL_HPP

class Signal
{
public:
    Signal();
    ~Signal();

    bool    setup();
    bool    shouldStop() const;
    void    requestStop();

private:
    Signal(const Signal &);
    Signal &operator=(const Signal &);

    bool    reportSystemError(const char *) const;
};

#endif
