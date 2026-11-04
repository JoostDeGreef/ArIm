#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

template<typename T, size_t BlockSize = 256>
class BlockVector
{
    struct Block
    {
        std::array<T, BlockSize> m_data; // array of elements in this block
        Block* m_prev = nullptr;         // pointer to the previous block in the list, or nullptr if this is the first block
        Block* m_next = nullptr;         // pointer to the next block in the list, or nullptr if this is the last block
        size_t m_used = 0;               // number of elements used in this block
        size_t m_order = 0;              // used for sorting block iterators
    };

    Block* m_first = nullptr;
    Block* m_last = nullptr;
    size_t m_used = 0;

public:
    BlockVector() = default;
    BlockVector(const BlockVector& other)
    {
        try
        {
            for(const T& value : other)
            {
                PushBack(value);
            }
        }
        catch(...)
        {
            Clear();
            throw;
        }
    }

    BlockVector& operator=(const BlockVector& other)
    {
        if(this != &other)
        {
            BlockVector copy(other);
            *this = std::move(copy);
        }
        return *this;
    }

    BlockVector(BlockVector&& other) noexcept
        : m_first(other.m_first), m_last(other.m_last), m_used(other.m_used)
    {
        other.m_first = nullptr;
        other.m_last = nullptr;
        other.m_used = 0;
    }

    BlockVector& operator=(BlockVector&& other) noexcept
    {
        if(this != &other)
        {
            Clear();
            m_first = other.m_first;
            m_last = other.m_last;
            m_used = other.m_used;
            other.m_first = nullptr;
            other.m_last = nullptr;
            other.m_used = 0;
        }
        return *this;
    }

    template<typename Iterator, bool IsConst, bool IsReverse = false>
    struct IteratorImpl
    {
        using BlockPointer = typename std::conditional<IsConst, const Block*, Block*>::type;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = typename std::conditional<IsConst, const T&, T&>::type;
        using pointer = typename std::conditional<IsConst, const T*, T*>::type;

        IteratorImpl(BlockPointer b, size_t index) : m_block(b), m_index(index) {}
        reference operator*() const { return m_block->m_data[IsReverse ? m_block->m_used - 1 - m_index : m_index]; }
        pointer operator->() const { return &m_block->m_data[IsReverse ? m_block->m_used - 1 - m_index : m_index]; }
        Iterator& operator++()
        {
            if(++m_index >= m_block->m_used) 
            { 
                m_block = IsReverse ? m_block->m_prev : m_block->m_next; 
                m_index = 0; 
            }
            return static_cast<Iterator&>(*this);
        }
        Iterator operator++(int)
        {
            Iterator previous = static_cast<Iterator&>(*this);
            ++(*this);
            return previous;
        }
        bool operator==(const Iterator& other) const
        {
            if(m_block == other.m_block)
            {
                return !m_block || other.m_index == m_index;
            }
            return false;
        }
        bool operator!=(const Iterator& other) const 
        { 
            return !(*this == other); 
        }
        bool operator < (const Iterator & other) const
        {
            if(m_block->m_order == other.m_block->m_order)
            {
                return m_index < other.m_index;
            }
            else
            {
                return m_block->m_order < other.m_block->m_order;
            }
        }
        bool operator > (const Iterator & other) const
        {
            return other < static_cast<const Iterator&>(*this);
        }

    private:
        friend class BlockVector;
        BlockPointer m_block;
        size_t m_index;
    };

    class Iterator : public IteratorImpl<Iterator, false>
    {
    public:
        using IteratorImpl<Iterator, false>::IteratorImpl;
    };

    class ConstIterator : public IteratorImpl<ConstIterator, true>
    {
    public:
        using IteratorImpl<ConstIterator, true>::IteratorImpl;
    };

    class ReverseIterator : public IteratorImpl<ReverseIterator, false, true>
    {
    public:
        using IteratorImpl<ReverseIterator, false, true>::IteratorImpl;
    };

    class ConstReverseIterator : public IteratorImpl<ConstReverseIterator, true, true>
    {
    public:
        using IteratorImpl<ConstReverseIterator, true, true>::IteratorImpl;
    };

    ConstIterator CBegin() const;
    ConstIterator CEnd() const;
    ConstIterator Begin() const;
    ConstIterator End() const;
    Iterator Begin();
    Iterator End();
    ConstIterator begin() const { return Begin(); }
    ConstIterator end() const { return End(); }
    Iterator begin() { return Begin(); }
    Iterator end() { return End(); }
    ReverseIterator RBegin();
    ReverseIterator REnd();
    ConstReverseIterator RBegin() const;
    ConstReverseIterator REnd() const;
    ReverseIterator rbegin() { return RBegin(); }
    ReverseIterator rend() { return REnd(); }
    ConstReverseIterator rbegin() const { return RBegin(); }
    ConstReverseIterator rend() const { return REnd(); }
    ConstReverseIterator CRBegin() const;
    ConstReverseIterator CREnd() const;
    ConstReverseIterator RCBegin() const;
    ConstReverseIterator RCEnd() const;

    ~BlockVector();

    void Clear();

    size_t Size() const;
    bool Empty() const;

    // insert a value as the last value
    void PushBack(const T& value);

    // return a reference to the last value
    T & Back()
    {
        assert(m_last != nullptr);
        return m_last->m_data[m_last->m_used - 1];  
    }
    const T & Back() const
    {
        assert(m_last != nullptr);
        return m_last->m_data[m_last->m_used - 1];  
    }
    T Pop();

    // erase one value at position and return the iterator to the next value
    Iterator Erase(Iterator position);
    ReverseIterator Erase(ReverseIterator position);

private:
};

template<typename T, size_t BlockSize>
T BlockVector<T, BlockSize>::Pop()
{
    assert(m_last != nullptr);
    T value = m_last->m_data[m_last->m_used - 1];
    --m_last->m_used;
    --m_used;
    if(!m_last->m_used)
    {
        Block* previous = m_last->m_prev;
        delete m_last;
        m_last = previous;
        if(m_last)
        {
            m_last->m_next = nullptr;
        }
        else
        {
            m_first = nullptr;
        }
    }
    return value;
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::Iterator BlockVector<T, BlockSize>::Begin()
{
    return Iterator(m_first, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::Iterator BlockVector<T, BlockSize>::End()
{
    return Iterator(nullptr, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstIterator BlockVector<T, BlockSize>::CBegin() const
{
    return ConstIterator(m_first, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstIterator BlockVector<T, BlockSize>::CEnd() const
{
    return ConstIterator(nullptr, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstIterator BlockVector<T, BlockSize>::Begin() const
{
    return ConstIterator(m_first, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstIterator BlockVector<T, BlockSize>::End() const
{
    return ConstIterator(nullptr, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ReverseIterator BlockVector<T, BlockSize>::RBegin()
{
    return ReverseIterator(m_last, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ReverseIterator BlockVector<T, BlockSize>::REnd()
{
    return ReverseIterator(nullptr, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstReverseIterator BlockVector<T, BlockSize>::RBegin() const
{
    return ConstReverseIterator(m_last, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstReverseIterator BlockVector<T, BlockSize>::REnd() const
{
    return ConstReverseIterator(nullptr, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstReverseIterator BlockVector<T, BlockSize>::CRBegin() const
{
    return ConstReverseIterator(m_last, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstReverseIterator BlockVector<T, BlockSize>::CREnd() const
{
    return ConstReverseIterator(nullptr, 0);
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstReverseIterator BlockVector<T, BlockSize>::RCBegin() const
{
    return CRBegin();
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ConstReverseIterator BlockVector<T, BlockSize>::RCEnd() const
{
    return CREnd();
}

template<typename T, size_t BlockSize>
BlockVector<T, BlockSize>::~BlockVector()
{
    Clear();
}

template<typename T, size_t BlockSize>
size_t BlockVector<T, BlockSize>::Size() const
{
    return m_used;
}

template<typename T, size_t BlockSize>
bool BlockVector<T, BlockSize>::Empty() const
{
    return !m_used;
}

template<typename T, size_t BlockSize>
void BlockVector<T, BlockSize>::Clear()
{
    Block * b = m_first;
    while(b)
    {             
        Block *t = b;           
        b = b->m_next;
        delete t;
    }
    m_first = nullptr;
    m_last = nullptr;
    m_used = 0;
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::Iterator BlockVector<T, BlockSize>::Erase(Iterator position)
{
    assert(position.m_block != nullptr);
    for(size_t i = position.m_index; i + 1 < position.m_block->m_used; ++i)
    {
        position.m_block->m_data[i] = position.m_block->m_data[i + 1];
    }
    --position.m_block->m_used;
    --m_used;
    if(!m_used)
    {
        Clear();
        position = end();
    }
    else if(!position.m_block->m_used)
    {
        Block * prev = position.m_block->m_prev;
        Block * next = position.m_block->m_next;
        delete position.m_block;
        if(!prev)
        {
            m_first = next;
        }
        else
        {
            prev->m_next = next;
        }
        if(!next)
        {
            m_last = prev;
        }
        else
        {
            next->m_prev = prev;
        }
        position.m_block = next;
        position.m_index = 0;
    }
    else if(position.m_index >= position.m_block->m_used)
    {
        position.m_index = 0;
        position.m_block = position.m_block->m_next;
    }
    return position;
}

template<typename T, size_t BlockSize>
typename BlockVector<T, BlockSize>::ReverseIterator BlockVector<T, BlockSize>::Erase(ReverseIterator position)
{
    assert(position.m_block != nullptr);
    Block * current = position.m_block;
    const size_t index = current->m_used - 1 - position.m_index;
    for(size_t i = index; i + 1 < current->m_used; ++i)
    {
        current->m_data[i] = current->m_data[i + 1];
    }
    --current->m_used;
    --m_used;
    if(!m_used)
    {
        Clear();
        return rend();
    }
    if(!current->m_used)
    {
        Block * prev = current->m_prev;
        Block * next = current->m_next;
        delete current;
        if(prev) prev->m_next = next; else m_first = next;
        if(next) next->m_prev = prev; else m_last = prev;
        return ReverseIterator(prev, 0);
    }
    if(index == 0)
    {
        return ReverseIterator(current->m_prev, 0);
    }
    return ReverseIterator(current, current->m_used - index);
}

template<typename T, size_t BlockSize>
void BlockVector<T, BlockSize>::PushBack(const T& value)
{
    if(!m_last)
    {
        m_last = new Block;
        m_first = m_last;
    }
    if( m_last->m_used == BlockSize)
    {
        m_last->m_next = new Block;
        m_last->m_next->m_prev = m_last;
        m_last = m_last->m_next;
        m_last->m_order = m_last->m_prev->m_order + 1;
    }
    m_last->m_data[m_last->m_used++] = value;
    ++m_used;
}
