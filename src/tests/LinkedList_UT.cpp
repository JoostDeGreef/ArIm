#include <utility>
#include <chrono>
#include <iostream>
#include <string>

#include "GoogleTestWrapper.h"
#include "ConsoleOutput.h"
#include "LinkedList.h"

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
    LinkedList<int, 2> values;

    EXPECT_TRUE(values.Empty());
    EXPECT_EQ(0, std::distance(values.begin(), values.end()));
    EXPECT_EQ(values.begin(), values.end());
    EXPECT_EQ(values.RBegin(), values.REnd());
}

TEST_F(LinkedListTest, PushBackAndIteratorsTraverseInBothDirections)
{
    LinkedList<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
    }

    ASSERT_FALSE(values.Empty());
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
    for(auto it = values.RBegin(); it != values.REnd(); ++it)
    {
        ASSERT_LT(index, 5);
        EXPECT_EQ(reverseExpected[index++], *it);
    }
    EXPECT_EQ(5, index);
}

TEST_F(LinkedListTest, EmplaceBackConstructsAndReturnsLastElement)
{
    struct Value
    {
        Value() : number(0) {}
        Value(int number, const std::string& text) : number(number), text(text) {}
        int number;
        std::string text;
    };

    LinkedList<Value, 2> values;
    Value& first = values.EmplaceBack(7, std::string("first"));
    values.EmplaceBack(8, "second");

    EXPECT_EQ(7, first.number);
    EXPECT_EQ("first", first.text);
    ASSERT_EQ(2, std::distance(values.begin(), values.end()));
    auto last = values.Last();
    EXPECT_EQ(8, last->number);
    EXPECT_EQ("second", last->text);
}

TEST_F(LinkedListTest, ConstIteratorsTraverseElements)
{
    LinkedList<int, 2> values;
    values.PushBack(6);
    values.PushBack(7);
    values.PushBack(8);

    const auto& constValues = values;
    const int expected[] = {6, 7, 8};
    int index = 0;
    for(auto it = constValues.CBegin(); it != constValues.CEnd(); ++it)
    {
        EXPECT_EQ(expected[index++], *it);
    }
    EXPECT_EQ(3, index);

    index = 0;
    for(auto it = constValues.CRBegin(); it != constValues.CREnd(); ++it)
    {
        EXPECT_EQ(8 - index++, *it);
    }
    EXPECT_EQ(3, index);
}

TEST_F(LinkedListTest, EraseFirstMiddleAndLastElements)
{
    LinkedList<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
    }

    auto next = values.Erase(values.begin());
    ASSERT_NE(values.end(), next);
    EXPECT_EQ(2, *next);

    auto middle = values.begin();
    ++middle;
    next = values.Erase(middle);
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
    EXPECT_EQ(values.end(), values.Erase(last));

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
    LinkedList<int, 2> values;
    values.PushBack(10);

    EXPECT_EQ(values.end(), values.Erase(values.begin()));
    EXPECT_TRUE(values.Empty());
    EXPECT_EQ(values.begin(), values.end());

    values.PushBack(20);
    ASSERT_EQ(1, std::distance(values.begin(), values.end()));
    EXPECT_EQ(20, *values.begin());
    EXPECT_EQ(values.end(), ++values.begin());
}

TEST_F(LinkedListTest, ReverseEraseReturnsPreviousElementAndUpdatesEndpoints)
{
    LinkedList<int, 2> values;
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
    LinkedList<int, 2> values;
    for(int value = 1; value <= 5; ++value)
    {
        values.PushBack(value);
    }

    auto position = values.RBegin();
    ++position; // 4; reverse replacement removes 4 and replaces its predecessor, 3
    auto inserted = values.ReplaceTwo(position, 30);
    ASSERT_NE(values.REnd(), inserted);
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
    LinkedList<int, 2> values;
    values.PushBack(1);
    values.PushBack(2);
    values.PushBack(3);

    values.Clear();
    EXPECT_TRUE(values.Empty());
    EXPECT_EQ(0, std::distance(values.begin(), values.end()));
    EXPECT_EQ(values.begin(), values.end());

    values.PushBack(4);
    ASSERT_EQ(1, std::distance(values.begin(), values.end()));
    EXPECT_EQ(4, *values.begin());
}

TEST_F(LinkedListTest, CopyOperationsCreateIndependentElements)
{
    LinkedList<int, 2> values;
    values.PushBack(1);
    values.PushBack(2);
    values.PushBack(3);

    LinkedList<int, 2> copied(values);
    EXPECT_EQ(1, *copied.begin());
    EXPECT_EQ(3, *copied.Last());
    *copied.begin() = 10;
    EXPECT_EQ(1, *values.begin());

    LinkedList<int, 2> assigned;
    assigned.PushBack(9);
    assigned = values;
    EXPECT_EQ(1, *assigned.begin());
    EXPECT_EQ(3, *assigned.Last());
    *assigned.begin() = 11;
    EXPECT_EQ(1, *values.begin());
}

TEST_F(LinkedListTest, MoveOperationsTransferNodesAndAllowSourceReuse)
{
    LinkedList<int, 2> values;
    values.PushBack(4);
    values.PushBack(5);
    values.PushBack(6);

    LinkedList<int, 2> moved(std::move(values));
    EXPECT_TRUE(values.Empty());
    EXPECT_EQ(4, *moved.begin());
    EXPECT_EQ(6, *moved.Last());
    values.PushBack(7);
    EXPECT_EQ(7, *values.begin());

    LinkedList<int, 2> assigned;
    assigned.PushBack(99);
    assigned = std::move(moved);
    EXPECT_TRUE(moved.Empty());
    EXPECT_EQ(4, *assigned.begin());
    EXPECT_EQ(6, *assigned.Last());
    moved.PushBack(8);
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

template<size_t BlockSize>
void RunLinkedListBlockSizeBenchmark()
{
    constexpr size_t ElementCount = 1000000;
    using Clock = std::chrono::steady_clock;
    LinkedList<int, BlockSize> values;

    const auto pushStart = Clock::now();
    for(size_t i = 0; i < ElementCount; ++i)
    {
        values.PushBack(static_cast<int>(i));
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
        position = values.ReplaceTwo(position, -1);
        ++position;
    }
    const auto replaceEnd = Clock::now();

    const auto pushMicros = std::chrono::duration_cast<std::chrono::microseconds>(pushEnd - pushStart).count();
    const auto replaceMicros = std::chrono::duration_cast<std::chrono::microseconds>(replaceEnd - replaceStart).count();
    MESSAGE() << "block_size=" << BlockSize
              << " push_back_us=" << pushMicros
              << " replace_two_us=" << replaceMicros;

    EXPECT_EQ((ElementCount + 1) / 2, static_cast<size_t>(std::distance(values.begin(), values.end())));
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
