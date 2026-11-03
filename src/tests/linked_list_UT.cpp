#include <utility>

#include "googletest_wrapper.h"
#include "linked_list.h"

class LinkedListTest : public Test 
{
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown() 
    {
    }

};

TEST_F(LinkedListTest, EmptyListHasNoElements)
{
    linked_list<int, 2> values;

    EXPECT_TRUE(values.empty());
    EXPECT_EQ(0u, values.size());
    EXPECT_EQ(values.begin(), values.end());
    EXPECT_EQ(values.rbegin(), values.rend());
}

TEST_F(LinkedListTest, PushBackAndIteratorsTraverseInBothDirections)
{
    linked_list<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }

    ASSERT_FALSE(values.empty());
    ASSERT_EQ(5u, values.size());
    const int expected[] = {1, 2, 3, 4, 5};
    int index = 0;
    for(int value : values)
    {
        ASSERT_LT(index, 5);
        EXPECT_EQ(expected[index++], value);
    }
    EXPECT_EQ(5, index);

    const int reverseExpected[] = {5, 4, 3, 2, 1};
    index = 0;
    for(auto it = values.rbegin(); it != values.rend(); ++it)
    {
        ASSERT_LT(index, 5);
        EXPECT_EQ(reverseExpected[index++], *it);
    }
    EXPECT_EQ(5, index);
}

TEST_F(LinkedListTest, ConstIteratorsTraverseElements)
{
    linked_list<int, 2> values;
    values.push_back(6);
    values.push_back(7);
    values.push_back(8);

    const auto& constValues = values;
    const int expected[] = {6, 7, 8};
    int index = 0;
    for(auto it = constValues.cbegin(); it != constValues.cend(); ++it)
    {
        EXPECT_EQ(expected[index++], *it);
    }
    EXPECT_EQ(3, index);

    index = 0;
    for(auto it = constValues.crbegin(); it != constValues.crend(); ++it)
    {
        EXPECT_EQ(8 - index++, *it);
    }
    EXPECT_EQ(3, index);
}

TEST_F(LinkedListTest, EraseFirstMiddleAndLastElements)
{
    linked_list<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }

    auto next = values.erase(values.begin());
    ASSERT_NE(values.end(), next);
    EXPECT_EQ(2, *next);

    auto middle = values.begin();
    ++middle;
    next = values.erase(middle);
    ASSERT_NE(values.end(), next);
    EXPECT_EQ(4, *next);

    auto last = values.begin();
    while(true)
    {
        auto following = last;
        ++following;
        if(following == values.end())
        {
            break;
        }
        last = following;
    }
    EXPECT_EQ(values.end(), values.erase(last));

    ASSERT_EQ(2u, values.size());
    const int expected[] = {2, 4};
    int index = 0;
    for(int value : values)
    {
        EXPECT_EQ(expected[index++], value);
    }
    EXPECT_EQ(2, index);
}

TEST_F(LinkedListTest, EraseOnlyElementAndReuseFreedNode)
{
    linked_list<int, 2> values;
    values.push_back(10);

    EXPECT_EQ(values.end(), values.erase(values.begin()));
    EXPECT_TRUE(values.empty());
    EXPECT_EQ(values.begin(), values.end());

    values.push_back(20);
    ASSERT_EQ(1u, values.size());
    EXPECT_EQ(20, *values.begin());
    EXPECT_EQ(values.end(), ++values.begin());
}

TEST_F(LinkedListTest, ReverseEraseReturnsPreviousElementAndUpdatesEndpoints)
{
    linked_list<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }

    auto previous = values.erase(values.rbegin());
    ASSERT_NE(values.rend(), previous);
    EXPECT_EQ(4, *previous);

    auto middle = values.rbegin();
    ++middle; // 3
    previous = values.erase(middle);
    ASSERT_NE(values.rend(), previous);
    EXPECT_EQ(2, *previous);

    auto first = values.rbegin();
    ++first; // 2
    ++first; // 1
    EXPECT_EQ(values.rend(), values.erase(first));

    ASSERT_EQ(2u, values.size());
    auto it = values.begin();
    ASSERT_NE(values.end(), it);
    EXPECT_EQ(2, *it++);
    ASSERT_NE(values.end(), it);
    EXPECT_EQ(4, *it++);
    EXPECT_EQ(values.end(), it);
}

TEST_F(LinkedListTest, ReverseReplaceTwoReplacesAdjacentElements)
{
    linked_list<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }

    auto position = values.rbegin();
    ++position; // 4; reverse replacement removes 4 and replaces its predecessor, 3
    auto inserted = values.replace_two(position, 30);
    ASSERT_NE(values.rend(), inserted);
    EXPECT_EQ(30, *inserted);

    const int expected[] = {1, 2, 30, 5};
    int index = 0;
    for(int value : values)
    {
        ASSERT_LT(index, 4);
        EXPECT_EQ(expected[index++], value);
    }
    EXPECT_EQ(4, index);
    EXPECT_EQ(4u, values.size());
}

TEST_F(LinkedListTest, ClearEmptiesListAndAllowsReuse)
{
    linked_list<int, 2> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    values.clear();
    EXPECT_TRUE(values.empty());
    EXPECT_EQ(0u, values.size());
    EXPECT_EQ(values.begin(), values.end());

    values.push_back(4);
    ASSERT_EQ(1u, values.size());
    EXPECT_EQ(4, *values.begin());
}
