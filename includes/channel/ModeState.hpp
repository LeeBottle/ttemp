#ifndef MODESTATE_HPP
# define MODESTATE_HPP

# include <cstddef>
# include <string>

class ModeState
{
public:
    ModeState();
    ~ModeState();

    const std::string   &topic() const;
    const std::string   &key() const;
    
    std::string modeString() const;
    std::string modeParameters() const;
    
    bool    inviteOnly() const;
    bool    topicRestricted() const;
    bool    hasKey() const;
    bool    hasLimit() const;
    size_t  limit() const;
    void    setTopic(const std::string &);
    bool    setInviteOnly(bool);
    bool    setTopicRestricted(bool);
    void    setKey(const std::string &);
    bool    clearKey();
    void    setLimit(size_t);
    bool    clearLimit();

private:
    std::string _topic;
    bool        _inviteOnly;
    bool        _topicRestricted;
    bool        _hasKey;
    std::string _key;
    bool        _hasLimit;
    size_t      _limit;

    ModeState(const ModeState &);
    ModeState &operator=(const ModeState &);
};

#endif
