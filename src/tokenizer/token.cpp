#include "token.h"

Token::Token()
    : count(0)
    , token(root)
    , left(root)
    , right(root)
    , type(token_type::Empty)
{}

Token::Token(const data_type token)
    : count(0)
    , token(token)
    , left(root)
    , right(root)
    , type(token_type::Root)
{}

Token::Token(const data_type token, const data_type left, const data_type right)
    : count(0)
    , token(token)
    , left(left)
    , right(right)
    , type(token_type::Node)
{}
