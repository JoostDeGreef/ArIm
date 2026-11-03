#pragma once

#include <unordered_set>

#include "token.h"

class TokenPair
{
public:
    Token left;     // left token of the pair
    Token right;    // right token of the pair

    TokenPair()
        : TokenPair(Token::root, Token::root)
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
            constexpr int shift = 8*(sizeof(size_t) - sizeof(Token::Data));
            return std::hash<size_t>()((size_t)tokenPair.left.token ^ ((size_t)tokenPair.right.token << shift));
        }
    };
};

