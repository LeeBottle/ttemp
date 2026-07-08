#ifndef PARSER_HPP
# define PARSER_HPP

# include <string>
# include <vector>

class Parser
{
public:
    enum Type
    {
        UNKNOWN,
        CAP,
        PING,
        PONG,
        QUIT,
        PASS,
        NICK,
        USER,
        JOIN,
        PART,
        PRIVMSG,
        NAMES,
        WHO,
        TOPIC,
        INVITE,
        KICK,
        MODE
    };

    Parser();
    ~Parser();

    const std::string               &name() const;
    const std::vector<std::string>  &params() const;

    Type    type() const;

    bool    parse(const std::string &);

private:
    std::string                 _name;
    std::vector<std::string>    _params;
    Type                        _type;

    Parser(const Parser &);
    Parser &operator=(const Parser &);
};

#endif
