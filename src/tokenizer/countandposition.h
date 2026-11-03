#pragma once

#include <vector>
#include <unordered_set>

#include "token.h"
#include "linked_list.h"

class CountAndPosition
{
public:
    size_t count;
    std::unordered_set<linked_list<Token>::iterator,
                       linked_list<Token>::iterator::Hash> positions;

    struct CountSortFunction
    {
        bool operator()(const CountAndPosition& a, const CountAndPosition& b) const
        {
            return a.count > b.count;
        }
    };
};
