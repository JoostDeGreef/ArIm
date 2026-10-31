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

TEST_F(BlockVectorTest, SmokeTest)
{
    block_vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    ASSERT_EQ(3,v.size());
    int j = 0;
    for(const auto & i : v)
    {
        j++;
        EXPECT_EQ(j, i);
    }
    v.clear();
    ASSERT_EQ(0,v.size());
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
