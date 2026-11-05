#pragma once

#include <unordered_set>

#include "Token.h"

class TokenPair
{
public:
    Token left;     // left token of the pair
    Token right;    // right token of the pair

    TokenPair()
        : left(Token::Root)
        , right(Token::Root)
    {}
    TokenPair(const Token left, const Token right)
        : left(left)
        , right(right)
    {}

    bool operator==(const TokenPair& other) const
    {
        return left == other.left && right == other.right;
    }
    bool operator!=(const TokenPair& other) const
    {
        return left != other.left || right != other.right;
    }
    bool operator<(const TokenPair& other) const
    {
        return left < other.left || (left == other.left && right < other.right);
    }
    bool operator>(const TokenPair& other) const
    {
        return left > other.left || (left == other.left && right > other.right);
    }

    struct HashFunction
    {
        size_t operator()(const TokenPair& tokenPair) const
        {
            constexpr int Shift = 8*(sizeof(size_t) - sizeof(Token::Data));
            return std::hash<size_t>()((size_t)tokenPair.left.GetToken() ^ ((size_t)tokenPair.right.GetToken() << Shift));
        }
    };
};

