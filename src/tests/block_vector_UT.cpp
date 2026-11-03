#include <utility>

#include "googletest_wrapper.h"
#include "block_vector.h"

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

TEST_F(BlockVectorTest, ReplaceTwoAcrossBlockBoundary)
{
    block_vector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.push_back(value);
    }
    auto position = values.begin();
    ++position;

    auto inserted = values.replace_two(position, 9);

    ASSERT_EQ(4, values.size());
    EXPECT_EQ(9, *inserted);
    const int expected[] = {1, 9, 4, 5};
    int index = 0;
    for(int value : values)
    {
        EXPECT_EQ(expected[index++], value);
    }
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

