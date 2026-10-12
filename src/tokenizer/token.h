#pragma once

#include <cstdint>

class Token
{
public:
    typedef uint16_t data_type;
    enum class token_type : data_type
    {
        Empty,
        Root,
        Node,
    };

    Token();
    Token(const data_type token); // add root
    Token(const data_type token, const data_type left, const data_type right); // add node

    Token(const Token &) = default;
    Token(Token &&) = default;
    Token & operator = (const Token &) = default;
    Token & operator = (Token &&) = default;

    static constexpr data_type root = (data_type)-1;

    uint32_t count;
    data_type token;
    data_type left;
    data_type right;
    token_type type;
};
