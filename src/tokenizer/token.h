#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

class Token
{
public:
    typedef uint16_t Data;

    Token();
    Token(const Data token);
    Token(const Data token,const Data left,const Data right);

    Token(const Token &) = default;
    Token(Token &&) = default;
    Token & operator = (const Token &) = default;
    Token & operator = (Token &&) = default;

    static constexpr Data root = (Data)-1;

    Data token;
    Data left; // todo: see if left+right can and should be removed here.
    Data right;
    mutable uint32_t count;
    mutable bool separator;

    bool operator == (const Token & other) const
    {
        return token == other.token;
    }

    struct HashFunction
    {
        size_t operator()(const Token& token) const
        {
            return std::hash<size_t>()(token.token);
        }
    };

};
