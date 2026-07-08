#ifndef RECEIVEBUFFER_HPP
# define RECEIVEBUFFER_HPP

# include <cstddef>
# include <string>

class ReceiveBuffer
{
public:
    ReceiveBuffer();
    ~ReceiveBuffer();

    void                append(const char *, size_t);
    const std::string   &data() const;
    bool                pop(std::string &);
    void                remove(size_t);

private:
    std::string _buffer;

    ReceiveBuffer(const ReceiveBuffer &);
    ReceiveBuffer &operator=(const ReceiveBuffer &);
};

#endif
