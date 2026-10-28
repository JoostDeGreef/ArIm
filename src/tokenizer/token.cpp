#include "token.h"

Token::Token()
    : Token(root)
{}

Token::Token(const Data token)
    : Token(token, root, root)
{}

Token::Token(const Data token, const Data left,const Data right)
    : token(token)
    , left(left)
    , right(right)
    , count(0)
    , separator(false)
{}

