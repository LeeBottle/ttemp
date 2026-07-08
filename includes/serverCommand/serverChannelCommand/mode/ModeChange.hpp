#ifndef MODECHANGE_HPP
# define MODECHANGE_HPP

# include <cstddef>
# include <string>
# include <vector>

class Client;

struct ModeOperation
{
    char        sign;
    char        mode;
    std::string value;
    size_t      limit;
    Client      *target;

    ModeOperation();
};

struct ModeChange
{
    std::string changes;
    std::string params;
    size_t      paramIndex;
    char        sign;
    char        currentSign;

    std::vector<ModeOperation>  operations;

    ModeChange();

    void    addChange(char);
};

#endif
