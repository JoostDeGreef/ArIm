#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

class Token
{
public:
    typedef uint16_t Data;

    static constexpr Data root = (Data)-1;                   // initial input sequence token
    static constexpr Data separated = 1<<(sizeof(Data)*8-1); // preceded by separating space

    Data token;

    Token()
        : Token(root)
    {}

    Token(const Data token)
        : token(token)
    {}

    Token(const Token &) = default;
    Token(Token &&) = default;
    Token & operator = (const Token &) = default;
    Token & operator = (Token &&) = default;

    bool operator == (const Token & other) const
    {
        return token == other.token;
    }
    bool operator != (const Token & other) const
    {
        return token != other.token;
    }
    bool operator < (const Token & other) const
    {
        return token < other.token;
    }
    bool operator > (const Token & other) const
    {
        return token > other.token;
    }

    Token & operator ++ ()
    {
        ++token;
        return *this;
    }
    Token & operator -- ()
    {
        --token;
        return *this;
    }
    Token operator ++ (int)
    {
        Token tmp(*this);
        ++token;
        return tmp;
    }
    Token operator -- (int)
    {
        Token tmp(*this);
        --token;
        return tmp;
    }

    bool isSeparator() const
    {
        return (token & separated) != 0;
    }
    void setSeparator()
    {
        token |= separated;
    }

    struct HashFunction
    {
        size_t operator()(const Token& token) const
        {
            return std::hash<size_t>()(token.token);
        }
    };
};
