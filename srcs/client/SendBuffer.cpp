#include "client/SendBuffer.hpp"


SendBuffer::SendBuffer() : _buffer()
{
}


SendBuffer::~SendBuffer()
{
}


void    SendBuffer::append(const std::string &message)
{
    _buffer.append(message);
}


bool    SendBuffer::hasData() const
{
    return (!_buffer.empty());
}


const char  *SendBuffer::data() const
{
    return (_buffer.data());
}


size_t  SendBuffer::size() const
{
    return (_buffer.size());
}


void    SendBuffer::remove(size_t length)
{
    _buffer.erase(0, length);
}
