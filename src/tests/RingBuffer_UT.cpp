#include <algorithm>
#include <vector>

#include "GoogleTestWrapper.h"
#include "RingBuffer.h"

TEST(RingBufferTest, AppendsAndIteratesLiveValues)
{
    RingBuffer<int> values;
    values.Reserve(4);
    values.PushBack(1, 2, 3);
    values.PushBack(4);

    EXPECT_EQ(4u, values.Size());
    EXPECT_TRUE(std::equal(values.begin(), values.end(), std::vector<int>{1, 2, 3, 4}.begin()));
}

TEST(RingBufferTest, ErasesFrontAndProvidesContiguousData)
{
    RingBuffer<int> values;
    values.Reserve(4);
    values.PushBack(1, 2, 3);
    values.EraseFront(2);

    ASSERT_EQ(1u, values.Size());
    ASSERT_NE(nullptr, values.GetData(1));
    EXPECT_EQ(3, values.GetData(1)[0]);
    EXPECT_EQ(nullptr, values.GetData(2));

    values.PushBack(4, 5, 6);
    const int expected[] = {3, 4, 5, 6};
    ASSERT_EQ(4u, values.Size());
    const int* data = values.GetData(4);
    ASSERT_NE(nullptr, data);
    EXPECT_TRUE(std::equal(data, data + 4, expected));
}

TEST(RingBufferTest, ClearResetsTheBuffer)
{
    RingBuffer<int> values;
    values.PushBack(7, 8);
    values.Clear();

    EXPECT_TRUE(values.Empty());
    EXPECT_EQ(0u, values.Size());
    EXPECT_EQ(values.begin(), values.end());
}

TEST(RingBufferTest, GrowsWhenUnreadValuesFillReservedStorage)
{
    RingBuffer<int> values;
    values.Reserve(2);
    values.PushBack(1, 2, 3);

    ASSERT_EQ(3u, values.Size());
    const int* data = values.GetData(3);
    ASSERT_NE(nullptr, data);
    EXPECT_EQ(1, data[0]);
    EXPECT_EQ(2, data[1]);
    EXPECT_EQ(3, data[2]);
}
