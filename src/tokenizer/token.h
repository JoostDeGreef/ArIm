#pragma once

#include <cstdint>

class Token
{
public:
    typedef uint16_t Data;
    enum class Type : Data
    {
        Empty,
        Root,
        Node,
    };

    Token();
    Token(const Data token); // add root
    Token(const Data token, const Data left, const Data right); // add node

    Token(const Token &) = default;
    Token(Token &&) = default;
    Token & operator = (const Token &) = default;
    Token & operator = (Token &&) = default;

    static constexpr Data root = (Data)-1;

    uint32_t count;
    Data token;
    Data left;
    Data right;
    Type type;
};
