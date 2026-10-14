#include "token.h"

Token::Token()
    : count(0)
    , token(root)
    , left(root)
    , right(root)
    , type(Type::Empty)
{}

Token::Token(const Data token)
    : count(0)
    , token(token)
    , left(root)
    , right(root)
    , type(Type::Root)
{}

Token::Token(const Data token, const Data left, const Data right)
    : count(0)
    , token(token)
    , left(left)
    , right(right)
    , type(Type::Node)
{}
