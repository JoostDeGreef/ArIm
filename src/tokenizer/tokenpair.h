#pragma once

#include <unordered_set>

#include "token.h"

class TokenPair
{
public:
    Token::Data left = Token::root;
    Token::Data right = Token::root;
    mutable uint32_t count = 0;

    TokenPair()
    {}
    TokenPair(const Token::Data left, const Token::Data right)
        : left(left)
        , right(right)
    {}
    TokenPair(const Token::Data left, const Token::Data right, const uint32_t count)
        : left(left)
        , right(right)
        , count(count)
    {}

    // operators which are only used for inserting in maps/sets.
    // they do explicitely not involve the count
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

    // sorter which sorts tokenpairs descending on count
    struct CountSort
    {
        bool operator()(const TokenPair & a, const TokenPair & b)
        {
            return a.count > b.count;
        }
    };
};

