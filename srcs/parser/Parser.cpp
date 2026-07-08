#include "parser/Parser.hpp"

#include <cctype>


static void skipSpaces(const std::string &line, size_t &index)
{
    while (index < line.size() && line[index] == ' ')
        ++index;
}


static std::string  toUpper(const std::string &value)
{
    std::string result;
    size_t      index;

    result = value;
    index = 0;
    while (index < result.size())
    {
        result[index] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(result[index])));
        ++index;
    }

    return (result);
}


static std::string  parseName(const std::string &line, size_t &index)
{
    size_t  start;

    start = index;
    while (index < line.size() && line[index] != ' ')
        ++index;

    return (toUpper(line.substr(start, index - start)));
}


static void parseParams(const std::string &line, size_t &index,
    std::vector<std::string> &params)
{
    size_t  start;

    while (index < line.size())
    {
        skipSpaces(line, index);
        if (index == line.size())
            break ;

        if (line[index] == ':')
        {
            params.push_back(line.substr(index + 1));
            break ;
        }

        start = index;
        while (index < line.size() && line[index] != ' ')
            ++index;

        params.push_back(line.substr(start, index - start));
    }
}


static Parser::Type    classifyType(const std::string &name)
{
    if (name == "CAP")
        return (Parser::CAP);

    if (name == "PING")
        return (Parser::PING);

    if (name == "PONG")
        return (Parser::PONG);

    if (name == "QUIT")
        return (Parser::QUIT);

    if (name == "PASS")
        return (Parser::PASS);

    if (name == "NICK")
        return (Parser::NICK);

    if (name == "USER")
        return (Parser::USER);

    if (name == "JOIN")
        return (Parser::JOIN);

    if (name == "PART")
        return (Parser::PART);

    if (name == "PRIVMSG")
        return (Parser::PRIVMSG);

    if (name == "NAMES")
        return (Parser::NAMES);

    if (name == "WHO")
        return (Parser::WHO);

    if (name == "TOPIC")
        return (Parser::TOPIC);

    if (name == "INVITE")
        return (Parser::INVITE);

    if (name == "KICK")
        return (Parser::KICK);

    if (name == "MODE")
        return (Parser::MODE);

    return (Parser::UNKNOWN);
}


Parser::Parser()
    : _name(),
      _params(),
      _type(UNKNOWN)
{
}


Parser::~Parser()
{
}


const std::string   &Parser::name() const
{
    return (_name);
}


const std::vector<std::string>  &Parser::params() const
{
    return (_params);
}


Parser::Type    Parser::type() const
{
    return (_type);
}


bool    Parser::parse(const std::string &line)
{
    size_t  index;

    _name.clear();
    _params.clear();
    _type = UNKNOWN;
    index = 0;

    skipSpaces(line, index);
    if (index == line.size())
        return (false);

    _name = parseName(line, index);
    _type = classifyType(_name);
    parseParams(line, index, _params);

    return (true);
}
