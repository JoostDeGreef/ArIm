#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>

template<typename T>
class RingBuffer
{
    template<bool IsConst>
    class IteratorImpl
    {
        using Buffer = typename std::conditional<IsConst, const RingBuffer, RingBuffer>::type;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = typename std::conditional<IsConst, const T&, T&>::type;
        using pointer = typename std::conditional<IsConst, const T*, T*>::type;

        IteratorImpl(Buffer* buffer, size_t index) : m_buffer(buffer), m_index(index) {}
        reference operator*() const { return m_buffer->m_data[m_index]; }
        pointer operator->() const { return &m_buffer->m_data[m_index]; }
        IteratorImpl& operator++()
        {
            m_index = (m_index + 1) % m_buffer->m_data.size();
            return *this;
        }
        IteratorImpl operator++(int) { auto previous = *this; ++*this; return previous; }
        bool operator==(const IteratorImpl& other) const
        {
            return m_buffer == other.m_buffer && m_index == other.m_index;
        }
        bool operator!=(const IteratorImpl& other) const { return !(*this == other); }

    private:
        Buffer* m_buffer;
        size_t m_index;
    };

public:
    using Iterator = IteratorImpl<false>;
    using ConstIterator = IteratorImpl<true>;

    size_t Size() const
    {
        return m_end >= m_start ? m_end - m_start : m_data.size() - m_start + m_end;
    }
    bool Empty() const { return m_start == m_end; }

    void Reserve(size_t size)
    {
        if(size + 1 > m_data.size())
        {
            Reallocate(size + 1);
        }
    }

    void Clear()
    {
        m_start = m_end = 0;
    }

    Iterator begin() { return Iterator(this, m_start); }
    Iterator end() { return Iterator(this, m_end); }
    ConstIterator begin() const { return ConstIterator(this, m_start); }
    ConstIterator end() const { return ConstIterator(this, m_end); }
    ConstIterator cbegin() const { return begin(); }
    ConstIterator cend() const { return end(); }

    void PushBack(const T& value)
    {
        if(m_data.empty())
        {
            Reallocate(2);
        }
        const size_t next = (m_end + 1) % m_data.size();
        if(next == m_start)
        {
            Reallocate(m_data.size() * 2);
        }
        m_data[m_end] = value;
        m_end = (m_end + 1) % m_data.size();
    }

    template<typename... Values>
    void PushBack(const T& value0, const T& value1, const Values&... values)
    {
        PushBack(value0);
        PushBack(value1);
        (PushBack(values), ...);
    }

    const T* GetData(size_t size)
    {
        if(size > Size())
        {
            return nullptr;
        }
        if(size != 0 && m_start + size > m_data.size())
        {
            Reallocate(m_data.size());
        }
        return m_data.empty() ? nullptr : m_data.data() + m_start;
    }

    void EraseFront(size_t size)
    {
        assert(size <= Size());
        if(size == Size())
        {
            m_start = m_end = 0;
            return;
        }
        m_start = (m_start + size) % m_data.size();
    }

private:
    void Reallocate(size_t capacity)
    {
        std::vector<T> data(capacity);
        const size_t size = Size();
        for(size_t i = 0; i < size; ++i)
        {
            data[i] = m_data[(m_start + i) % m_data.size()];
        }
        m_data.swap(data);
        m_start = 0;
        m_end = size;
    }

    std::vector<T> m_data;
    size_t m_start = 0;
    size_t m_end = 0;
};
