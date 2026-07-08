#ifndef SENDBUFFER_HPP
# define SENDBUFFER_HPP

# include <cstddef>
# include <string>

class SendBuffer
{
public:
    SendBuffer();
    ~SendBuffer();

    void        append(const std::string &);
    bool        hasData() const;
    const char  *data() const;
    size_t      size() const;
    void        remove(size_t);

private:
    std::string _buffer;

    SendBuffer(const SendBuffer &);
    SendBuffer &operator=(const SendBuffer &);
};

#endif
