#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <functional>

class Token
{
public:
    typedef uint16_t Data;

    static constexpr Data root = (Data)-1;                    // initial input sequence token
    static constexpr Data sepatorBit = 1<<(sizeof(Data)*8-1); // preceded by separating space
    static constexpr Data numberMask = root >> 1;             // mask for the token number

private:
    Data data;

public:
    Token()
        : data(root)
    {}

    Token(const Data token, const bool separator = false)
    {
        setToken(token, separator);
    }

    Token(const Token &) = default;
    Token(Token &&) = default;
    Token & operator = (const Token &) = default;
    Token & operator = (Token &&) = default;

    bool operator == (const Token & other) const
    {
        return data == other.data;
    }
    bool operator != (const Token & other) const
    {
        return data != other.data;
    }
    bool operator < (const Token & other) const
    {
        return data < other.data;
    }
    bool operator > (const Token & other) const
    {
        return data > other.data;
    }

    Token & operator ++ ()
    {
        assert(data < numberMask);
        ++data;
        return *this;
    }
    Token & operator -- ()
    {
        assert(data > 0);
        --data;
        return *this;
    }
    Token operator ++ (int)
    {
        Token tmp(*this);
        ++data;
        return tmp;
    }
    Token operator -- (int)
    {
        Token tmp(*this);
        --data;
        return tmp;
    }

    bool isSeparator() const
    {
        return (data & sepatorBit) != 0;
    }
    void setSeparator()
    {
        data |= sepatorBit;
    }
    Data getToken() const
    {
        return data & numberMask;
    }
    void setToken(Data token)
    {
        data = token;
    }
    void setToken(Data token, bool separator)
    {
        data = token | (separator ? sepatorBit : 0);
    }
    Data getData() const
    {
        return data;
    }
    void setData(Data value)
    {
        data = value;
    }

    struct HashFunction
    {
        size_t operator()(const Token& token) const
        {
            return std::hash<size_t>()(token.data);
        }
    };
};
