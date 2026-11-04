#include <utility>

#include "GoogleTestWrapper.h"
#include "BlockVector.h"

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
    BlockVector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
    }

    const int expected[] = {5, 4, 3, 2, 1};
    int index = 0;
    for(auto it = values.RBegin(); it != values.REnd(); ++it)
    {
        EXPECT_EQ(expected[index++], *it);
    }
    EXPECT_EQ(5, index);

    const auto& constValues = values;
    index = 0;
    for(auto it = constValues.CRBegin(); it != constValues.CREnd(); ++it)
    {
        EXPECT_EQ(expected[index++], *it);
    }
    EXPECT_EQ(5, index);
}

TEST_F(BlockVectorTest, ConstForwardAndReverseAccessorsTraverseValues)
{
    BlockVector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
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
    checkForward(constValues.CBegin(), constValues.CEnd());
    checkReverse(constValues.RBegin(), constValues.REnd());
    checkReverse(constValues.CRBegin(), constValues.CREnd());
    checkReverse(constValues.RCBegin(), constValues.RCEnd());
}

TEST_F(BlockVectorTest, IteratorsCompareAndPostIncrementWithinAndAcrossBlocks)
{
    BlockVector<int, 2> values;
    for(int value = 1; value <= 4; ++value)
    {
        values.PushBack(value);
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
    BlockVector<BlockVectorTestValue, 2> values;
    values.PushBack({7});

    EXPECT_EQ(7, values.begin()->value);
    const auto& constValues = values;
    EXPECT_EQ(7, constValues.begin()->value);
}

TEST_F(BlockVectorTest, ClearEmptiesVectorAndAllowsReuse)
{
    BlockVector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
    }

    values.Clear();
    EXPECT_TRUE(values.Empty());
    EXPECT_EQ(0, values.Size());
    EXPECT_EQ(values.begin(), values.end());

    values.PushBack(9);
    ASSERT_FALSE(values.Empty());
    EXPECT_EQ(1, values.Size());
    EXPECT_EQ(9, values.Back());
}

TEST_F(BlockVectorTest, EraseReturnsNextElementAcrossBlockBoundary)
{
    BlockVector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
    }

    auto next = values.Erase(++values.begin());

    ASSERT_EQ(4, values.Size());
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
    BlockVector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
    }

    auto firstNext = values.Erase(values.begin());
    ASSERT_NE(values.end(), firstNext);
    EXPECT_EQ(2, *firstNext);

    auto last = values.begin();
    for(int i = 0; i < 3; ++i)
    {
        ++last;
    }
    EXPECT_EQ(5, *last);
    EXPECT_EQ(values.end(), values.Erase(last));
    ASSERT_EQ(3, values.Size());

    const int expected[] = {2, 3, 4};
    int index = 0;
    for(int value : values)
    {
        EXPECT_EQ(expected[index++], value);
    }
}

TEST_F(BlockVectorTest, EraseOnlyElementLeavesVectorEmpty)
{
    BlockVector<int, 2> values;
    values.PushBack(7);

    EXPECT_EQ(values.end(), values.Erase(values.begin()));
    EXPECT_TRUE(values.Empty());
    EXPECT_EQ(values.begin(), values.end());
}

TEST_F(BlockVectorTest, ReverseEraseReturnsPreviousElementAcrossBlockBoundary)
{
    BlockVector<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
    }

    auto previous = values.Erase(values.RBegin());
    ASSERT_NE(values.REnd(), previous);
    EXPECT_EQ(4, *previous);

    auto middle = values.RBegin();
    ++middle; // 3
    previous = values.Erase(middle);
    ASSERT_NE(values.REnd(), previous);
    EXPECT_EQ(2, *previous);

    auto first = values.RBegin();
    ++first; // 2
    ++first; // 1
    EXPECT_EQ(values.REnd(), values.Erase(first));
    ASSERT_EQ(2u, values.Size());
    auto it = values.begin();
    EXPECT_EQ(2, *it++);
    EXPECT_EQ(4, *it++);
    EXPECT_EQ(values.end(), it);
}

TEST_F(BlockVectorTest, BackReturnsLastElement)
{
    BlockVector<int, 2> values;
    values.PushBack(1);
    values.PushBack(2);
    values.PushBack(3);

    EXPECT_EQ(3, values.Back());

    values.Back() = 9;
    EXPECT_EQ(9, values.Back());
    EXPECT_EQ(3, values.Size());
}

TEST_F(BlockVectorTest, ConstBackReturnsLastElement)
{
    BlockVector<int, 2> values;
    values.PushBack(4);
    values.PushBack(5);

    const BlockVector<int, 2>& constValues = values;
    EXPECT_EQ(5, constValues.Back());
}

TEST_F(BlockVectorTest, CopyOperationsCreateIndependentElements)
{
    BlockVector<int, 2> values;
    values.PushBack(1);
    values.PushBack(2);
    values.PushBack(3);

    BlockVector<int, 2> copied(values);
    ASSERT_EQ(3u, copied.Size());
    EXPECT_EQ(1, *copied.begin());
    EXPECT_EQ(3, copied.Back());
    *copied.begin() = 10;
    EXPECT_EQ(1, *values.begin());

    BlockVector<int, 2> assigned;
    assigned.PushBack(9);
    assigned = values;
    ASSERT_EQ(3u, assigned.Size());
    EXPECT_EQ(1, *assigned.begin());
    EXPECT_EQ(3, assigned.Back());
    *assigned.begin() = 11;
    EXPECT_EQ(1, *values.begin());
}

TEST_F(BlockVectorTest, PopRemovesValuesAndEmptyBlocks)
{
    BlockVector<int, 2> values;
    values.PushBack(1);
    values.PushBack(2);
    values.PushBack(3);

    EXPECT_EQ(3, values.Pop());
    EXPECT_EQ(2u, values.Size());
    EXPECT_EQ(2, values.Back());
    EXPECT_EQ(2, values.Pop());
    EXPECT_EQ(1, values.Pop());
    EXPECT_TRUE(values.Empty());
    EXPECT_EQ(values.begin(), values.end());

    values.PushBack(4);
    EXPECT_EQ(4, values.Pop());
    EXPECT_TRUE(values.Empty());
}

