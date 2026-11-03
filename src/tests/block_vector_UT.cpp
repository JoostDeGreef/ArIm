#include <utility>

#include "googletest_wrapper.h"
#include "block_vector.h"

struct BlockVectorTestValue
{
    int value;
};

class BlockVectorTest : public Test 
{
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown() 
    {
    }

};

TEST_F(BlockVectorTest, ReverseIteratorsTraverseAcrossBlocks)
{
    block_vector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }

    const int expected[] = {5, 4, 3, 2, 1};
    int index = 0;
    for(auto it = values.rbegin(); it != values.rend(); ++it)
    {
        EXPECT_EQ(expected[index++], *it);
    }
    EXPECT_EQ(5, index);

    const auto& constValues = values;
    index = 0;
    for(auto it = constValues.crbegin(); it != constValues.crend(); ++it)
    {
        EXPECT_EQ(expected[index++], *it);
    }
    EXPECT_EQ(5, index);
}

TEST_F(BlockVectorTest, ConstForwardAndReverseAccessorsTraverseValues)
{
    block_vector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }
    const auto& constValues = values;

    auto checkForward = [](auto first, auto last)
    {
        const int expected[] = {1, 2, 3, 4, 5};
        int index = 0;
        for(; first != last; ++first)
        {
            ASSERT_LT(index, 5);
            EXPECT_EQ(expected[index++], *first);
        }
        EXPECT_EQ(5, index);
    };
    auto checkReverse = [](auto first, auto last)
    {
        const int expected[] = {5, 4, 3, 2, 1};
        int index = 0;
        for(; first != last; ++first)
        {
            ASSERT_LT(index, 5);
            EXPECT_EQ(expected[index++], *first);
        }
        EXPECT_EQ(5, index);
    };

    checkForward(constValues.begin(), constValues.end());
    checkForward(constValues.cbegin(), constValues.cend());
    checkReverse(constValues.rbegin(), constValues.rend());
    checkReverse(constValues.crbegin(), constValues.crend());
    checkReverse(constValues.rcbegin(), constValues.rcend());
}

TEST_F(BlockVectorTest, IteratorsCompareAndPostIncrementWithinAndAcrossBlocks)
{
    block_vector<int, 2> values;
    for(int value = 1; value <= 4; ++value)
    {
        values.push_back(value);
    }

    auto first = values.begin();
    auto second = first;
    EXPECT_EQ(1, *second++);
    EXPECT_EQ(2, *second);
    EXPECT_LT(first, second);
    EXPECT_GT(second, first);
    EXPECT_NE(first, second);

    ++second;
    EXPECT_EQ(3, *second);
    EXPECT_LT(first, second);
    EXPECT_GT(second, first);
}

TEST_F(BlockVectorTest, IteratorArrowAccessesElementMembers)
{
    block_vector<BlockVectorTestValue, 2> values;
    values.push_back({7});

    EXPECT_EQ(7, values.begin()->value);
    const auto& constValues = values;
    EXPECT_EQ(7, constValues.begin()->value);
}

TEST_F(BlockVectorTest, ClearEmptiesVectorAndAllowsReuse)
{
    block_vector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }

    values.clear();
    EXPECT_TRUE(values.empty());
    EXPECT_EQ(0, values.size());
    EXPECT_EQ(values.begin(), values.end());

    values.push_back(9);
    ASSERT_FALSE(values.empty());
    EXPECT_EQ(1, values.size());
    EXPECT_EQ(9, values.back());
}

TEST_F(BlockVectorTest, EraseReturnsNextElementAcrossBlockBoundary)
{
    block_vector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }

    auto next = values.erase(++values.begin());

    ASSERT_EQ(4, values.size());
    ASSERT_NE(values.end(), next);
    EXPECT_EQ(3, *next);
    const int expected[] = {1, 3, 4, 5};
    int index = 0;
    for(int value : values)
    {
        EXPECT_EQ(expected[index++], value);
    }
}

TEST_F(BlockVectorTest, EraseRemovesEmptyFirstAndLastBlocks)
{
    block_vector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }

    auto firstNext = values.erase(values.begin());
    ASSERT_NE(values.end(), firstNext);
    EXPECT_EQ(2, *firstNext);

    auto last = values.begin();
    for(int i = 0; i < 3; ++i)
    {
        ++last;
    }
    EXPECT_EQ(5, *last);
    EXPECT_EQ(values.end(), values.erase(last));
    ASSERT_EQ(3, values.size());

    const int expected[] = {2, 3, 4};
    int index = 0;
    for(int value : values)
    {
        EXPECT_EQ(expected[index++], value);
    }
}

TEST_F(BlockVectorTest, EraseOnlyElementLeavesVectorEmpty)
{
    block_vector<int, 2> values;
    values.push_back(7);

    EXPECT_EQ(values.end(), values.erase(values.begin()));
    EXPECT_TRUE(values.empty());
    EXPECT_EQ(values.begin(), values.end());
}

TEST_F(BlockVectorTest, ReverseEraseReturnsPreviousElementAcrossBlockBoundary)
{
    block_vector<int, 2> values;
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
    EXPECT_EQ(2, *it++);
    EXPECT_EQ(4, *it++);
    EXPECT_EQ(values.end(), it);
}

TEST_F(BlockVectorTest, BackReturnsLastElement)
{
    block_vector<int, 2> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    EXPECT_EQ(3, values.back());

    values.back() = 9;
    EXPECT_EQ(9, values.back());
    EXPECT_EQ(3, values.size());
}

TEST_F(BlockVectorTest, ConstBackReturnsLastElement)
{
    block_vector<int, 2> values;
    values.push_back(4);
    values.push_back(5);

    const block_vector<int, 2>& constValues = values;
    EXPECT_EQ(5, constValues.back());
}

TEST_F(BlockVectorTest, PopRemovesValuesAndEmptyBlocks)
{
    block_vector<int, 2> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    EXPECT_EQ(3, values.pop());
    EXPECT_EQ(2u, values.size());
    EXPECT_EQ(2, values.back());
    EXPECT_EQ(2, values.pop());
    EXPECT_EQ(1, values.pop());
    EXPECT_TRUE(values.empty());
    EXPECT_EQ(values.begin(), values.end());

    values.push_back(4);
    EXPECT_EQ(4, values.pop());
    EXPECT_TRUE(values.empty());
}

