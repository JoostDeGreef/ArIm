#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <functional>

class Token
{
public:
    typedef uint16_t Data;

    static constexpr Data Root = (Data)-1;  // initial input sequence token

private:
    Data m_data;

public:
    Token()
        : m_data(Root)
    {}

    Token(const Data token)
    {
        SetToken(token);
    }

    Token(const Token &) = default;
    Token(Token &&) = default;
    Token & operator = (const Token &) = default;
    Token & operator = (Token &&) = default;

    bool operator == (const Token & other) const
    {
        return m_data == other.m_data;
    }
    bool operator != (const Token & other) const
    {
        return m_data != other.m_data;
    }
    bool operator < (const Token & other) const
    {
        return m_data < other.m_data;
    }
    bool operator > (const Token & other) const
    {
        return m_data > other.m_data;
    }

    Token & operator ++ ()
    {
        assert(m_data < Root);
        ++m_data;
        return *this;
    }
    Token & operator -- ()
    {
        assert(m_data > 0);
        --m_data;
        return *this;
    }
    Token operator ++ (int)
    {
        Token tmp(*this);
        ++m_data;
        return tmp;
    }
    Token operator -- (int)
    {
        Token tmp(*this);
        --m_data;
        return tmp;
    }

    bool IsSeparator() const
    {
        return m_data >=256 && m_data < 512;
    }
    bool IsCharacter() const
    {
        return m_data < 512;
    }
    char GetCharacter() const
    {
        return (char)(m_data & 255);
    }
    Data GetToken() const
    {
        return m_data;
    }
    void SetToken(Data token)
    {
        m_data = token;
    }

    struct HashFunction
    {
        size_t operator()(const Token& token) const
        {
            return std::hash<size_t>()(token.m_data);
        }
    };
};
