#include <utility>
#include <chrono>
#include <iostream>

#include "googletest_wrapper.h"
#include "ConsoleOutput.h"
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
    EXPECT_EQ(0, std::distance(values.begin(), values.end()));
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
    ASSERT_EQ(5, std::distance(values.begin(), values.end()));
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

    ASSERT_EQ(2, std::distance(values.begin(), values.end()));
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
    ASSERT_EQ(1, std::distance(values.begin(), values.end()));
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

    ASSERT_EQ(2, std::distance(values.begin(), values.end()));
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
    EXPECT_EQ(4, std::distance(values.begin(), values.end()));
}

TEST_F(LinkedListTest, ClearEmptiesListAndAllowsReuse)
{
    linked_list<int, 2> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    values.clear();
    EXPECT_TRUE(values.empty());
    EXPECT_EQ(0, std::distance(values.begin(), values.end()));
    EXPECT_EQ(values.begin(), values.end());

    values.push_back(4);
    ASSERT_EQ(1, std::distance(values.begin(), values.end()));
    EXPECT_EQ(4, *values.begin());
}

TEST_F(LinkedListTest, CopyOperationsCreateIndependentElements)
{
    linked_list<int, 2> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    linked_list<int, 2> copied(values);
    EXPECT_EQ(1, *copied.begin());
    EXPECT_EQ(3, *copied.last());
    *copied.begin() = 10;
    EXPECT_EQ(1, *values.begin());

    linked_list<int, 2> assigned;
    assigned.push_back(9);
    assigned = values;
    EXPECT_EQ(1, *assigned.begin());
    EXPECT_EQ(3, *assigned.last());
    *assigned.begin() = 11;
    EXPECT_EQ(1, *values.begin());
}

TEST_F(LinkedListTest, MoveOperationsTransferNodesAndAllowSourceReuse)
{
    linked_list<int, 2> values;
    values.push_back(4);
    values.push_back(5);
    values.push_back(6);

    linked_list<int, 2> moved(std::move(values));
    EXPECT_TRUE(values.empty());
    EXPECT_EQ(4, *moved.begin());
    EXPECT_EQ(6, *moved.last());
    values.push_back(7);
    EXPECT_EQ(7, *values.begin());

    linked_list<int, 2> assigned;
    assigned.push_back(99);
    assigned = std::move(moved);
    EXPECT_TRUE(moved.empty());
    EXPECT_EQ(4, *assigned.begin());
    EXPECT_EQ(6, *assigned.last());
    moved.push_back(8);
    EXPECT_EQ(8, *moved.begin());
}

class LinkedListBlockSizeBenchmark : public TestWithParam<size_t>
{
public:
    static void SetUpTestSuite()
    {
        ConsoleOutput::SetCompactTestOutput(true);
    }

    static void TearDownTestSuite()
    {
        ConsoleOutput::SetCompactTestOutput(false);
    }
};

template<size_t blockSize>
void RunLinkedListBlockSizeBenchmark()
{
    constexpr size_t elementCount = 1000000;
    using Clock = std::chrono::steady_clock;
    linked_list<int, blockSize> values;

    const auto pushStart = Clock::now();
    for(size_t i = 0; i < elementCount; ++i)
    {
        values.push_back(static_cast<int>(i));
    }
    const auto pushEnd = Clock::now();

    const auto replaceStart = Clock::now();
    auto position = values.begin();
    while(position != values.end())
    {
        auto next = position;
        ++next;
        if(next == values.end())
        {
            break;
        }
        position = values.replace_two(position, -1);
        ++position;
    }
    const auto replaceEnd = Clock::now();

    const auto pushMicros = std::chrono::duration_cast<std::chrono::microseconds>(pushEnd - pushStart).count();
    const auto replaceMicros = std::chrono::duration_cast<std::chrono::microseconds>(replaceEnd - replaceStart).count();
    MESSAGE() << "block_size=" << blockSize
              << " push_back_us=" << pushMicros
              << " replace_two_us=" << replaceMicros;

    EXPECT_EQ((elementCount + 1) / 2, static_cast<size_t>(std::distance(values.begin(), values.end())));
}

TEST_P(LinkedListBlockSizeBenchmark, DISABLED_PushBackAndReplaceTwo)
{
    switch(GetParam())
    {
        case 32: RunLinkedListBlockSizeBenchmark<32>(); break;
        case 64: RunLinkedListBlockSizeBenchmark<64>(); break;
        case 128: RunLinkedListBlockSizeBenchmark<128>(); break;
        case 256: RunLinkedListBlockSizeBenchmark<256>(); break;
        case 512: RunLinkedListBlockSizeBenchmark<512>(); break;
        case 1024: RunLinkedListBlockSizeBenchmark<1024>(); break;
        default: FAIL() << "Unexpected block size: " << GetParam();
    }
}

INSTANTIATE_TEST_SUITE_P(BlockSizes, LinkedListBlockSizeBenchmark,
                         Values(32, 64, 128, 256, 512, 1024));
