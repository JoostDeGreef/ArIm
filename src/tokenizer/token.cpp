#include "token.h"

Token::Token()
    : Token(root)
{}

Token::Token(const Data token)
    : count(0)
    , token(token)
    , separator(false)
{}
