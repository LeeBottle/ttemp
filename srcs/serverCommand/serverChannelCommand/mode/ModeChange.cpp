#include "serverCommand/serverChannelCommand/mode/ModeChange.hpp"

#include <cstddef>


ModeOperation::ModeOperation()
    : sign('+'),
      mode('\0'),
      value(),
      limit(0),
      target(NULL)
{
}


ModeChange::ModeChange()
    : changes(),
      params(),
      paramIndex(2),
      sign('+'),
      operations(),
      currentSign('\0')
{
}


void    ModeChange::addChange(char mode)
{
    if (currentSign != sign)
    {
        changes += sign;
        currentSign = sign;
    }

    changes += mode;
}
