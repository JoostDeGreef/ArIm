#pragma once

#include <array>
#include <cassert>
#include <memory>

template<typename T, size_t block_size = 256>
class block_vector
{
    struct block
    {
        std::array<T, block_size> m_data;
        block* m_prev = nullptr;
        block* m_next = nullptr;
        size_t m_used = 0;
    };

    block* m_first = nullptr;
    block* m_last = nullptr;
    size_t m_used = 0;

public:
    class iterator
    {
        friend block_vector;
    public:
        iterator(block * b, size_t index) : m_block(b), m_index(index) {}
        T& operator*() const { return m_block->m_data[m_index]; }
        T* operator->() const { return &m_block->m_data[m_index]; }
        iterator& operator++() { if(++m_index>=m_block->m_used) { m_block = m_block->m_next; m_index = 0; }; return *this; }
        bool operator==(const iterator& other) const
        { return (m_block == other.m_block) && (!m_block || (other.m_index == m_index)); }
        bool operator!=(const iterator& other) const
        { return !(*this == other); }
    private:
        block* m_block;
        size_t m_index;
    };

    iterator begin();
    iterator end();

    ~block_vector();

    void clear();

    size_t size() const;
    bool empty() const;

    // insert a value as the last value
    void push_back(const T& value);

    // erase one value at position and return the iterator to the next value
    iterator erase(iterator position);

    // insert one value at position, replacing two values, return iterator to inserted value
    iterator replace_two(iterator position, const T & value);

private:
};

template<typename T, size_t block_size>
typename block_vector<T, block_size>::iterator block_vector<T, block_size>::begin()
{
    return iterator(m_first, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::iterator block_vector<T, block_size>::end()
{
    return iterator(nullptr, 0);
}

template<typename T, size_t block_size>
block_vector<T, block_size>::~block_vector()
{
    clear();
}

template<typename T, size_t block_size>
size_t block_vector<T, block_size>::size() const
{
    return m_used;
}

template<typename T, size_t block_size>
bool block_vector<T, block_size>::empty() const
{
    return !m_used;
}

template<typename T, size_t block_size>
void block_vector<T, block_size>::clear()
{
    block * b = m_first;
    while(b)
    {             
        block *t = b;           
        b = b->m_next;
        delete t;
    }
    m_first = nullptr;
    m_last = nullptr;
    m_used = 0;
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::iterator block_vector<T, block_size>::erase(iterator position)
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
        clear();
        position = end();
    }
    else if(!position.m_block->m_used)
    {
        block * prev = position.m_block->m_prev;
        block * next = position.m_block->m_next;
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

template<typename T, size_t block_size>
typename block_vector<T, block_size>::iterator block_vector<T, block_size>::replace_two(iterator position, const T & value)
{
    assert(position.m_block != nullptr);
    assert(m_used >= 2);
    position = erase(position);
    *position = value;
    return position;
}

template<typename T, size_t block_size>
void block_vector<T, block_size>::push_back(const T& value)
{
    if(!m_last)
    {
        m_last = new block;
        m_first = m_last;
    }
    if( m_last->m_used == block_size)
    {
        m_last->m_next = new block;
        m_last->m_next->m_prev = m_last;
        m_last = m_last->m_next;
    }
    m_last->m_data[m_last->m_used++] = value;
    ++m_used;
}
