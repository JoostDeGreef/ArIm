#pragma once

#include <unordered_set>

#include "token.h"

class TokenPair
{
public:
    mutable Token::Data token;
    Token::Data left;
    Token::Data right;

    TokenPair()
        : TokenPair(Token::root, Token::root, Token::root)
    {}
    TokenPair(const Token::Data token, const Token::Data left, const Token::Data right)
        : token(token)
        , left(left)
        , right(right)
    {}

    bool operator==(const TokenPair& other) const
    {
        return left == other.left && right == other.right;
    }

    struct HashFunction
    {
        size_t operator()(const TokenPair& tokenPair) const
        {
            constexpr int shift = 8*(sizeof(size_t) - sizeof(Token::Data));
            return std::hash<size_t>()((size_t)tokenPair.left ^ ((size_t)tokenPair.right << shift));
        }
    };
};

