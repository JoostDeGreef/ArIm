#pragma once

#include <vector>
#include <unordered_set>

#include "Token.h"
#include "LinkedList.h"

class CountAndPosition
{
public:
    size_t count;
    std::unordered_set<LinkedList<Token>::Iterator,
                       LinkedList<Token>::Iterator::Hash> positions;

    struct CountSortFunction
    {
        bool operator()(const CountAndPosition& a, const CountAndPosition& b) const
        {
            return a.count > b.count;
        }
    };
};
