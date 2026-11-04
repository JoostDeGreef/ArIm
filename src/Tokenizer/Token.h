#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <functional>

class Token
{
public:
    typedef uint16_t Data;

    static constexpr Data Root = (Data)-1;                    // initial input sequence token
    static constexpr Data SeparatorBit = 1<<(sizeof(Data)*8-1); // preceded by separating space
    static constexpr Data NumberMask = Root >> 1;             // mask for the token number

private:
    Data m_data;

public:
    Token()
        : m_data(Root)
    {}

    Token(const Data token, const bool separator = false)
    {
        SetToken(token, separator);
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
        assert(m_data < NumberMask);
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
        return (m_data & SeparatorBit) != 0;
    }
    void SetSeparator()
    {
        m_data |= SeparatorBit;
    }
    Data GetToken() const
    {
        return m_data & NumberMask;
    }
    void SetToken(Data token)
    {
        m_data = token;
    }
    void SetToken(Data token, bool separator)
    {
        m_data = token | (separator ? SeparatorBit : 0);
    }
    Data GetData() const
    {
        return m_data;
    }
    void SetData(Data value)
    {
        m_data = value;
    }

    struct HashFunction
    {
        size_t operator()(const Token& token) const
        {
            return std::hash<size_t>()(token.m_data);
        }
    };
};
