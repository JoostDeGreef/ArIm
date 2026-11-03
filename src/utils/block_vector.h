#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

template<typename T, size_t block_size = 256>
class block_vector
{
    struct block
    {
        std::array<T, block_size> m_data; // array of elements in this block
        block* m_prev = nullptr;          // pointer to the previous block in the list, or nullptr if this is the first block
        block* m_next = nullptr;          // pointer to the next block in the list, or nullptr if this is the last block
        size_t m_used = 0;                // number of elements used in this block
        size_t m_order = 0;               // used for sorting block iterators
    };

    block* m_first = nullptr;
    block* m_last = nullptr;
    size_t m_used = 0;

public:
    template<typename ITERATOR, bool IS_CONST, bool IS_REVERSE = false>
    struct iterator_impl
    {
        using block_pointer = typename std::conditional<IS_CONST, const block*, block*>::type;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = typename std::conditional<IS_CONST, const T&, T&>::type;
        using pointer = typename std::conditional<IS_CONST, const T*, T*>::type;

        iterator_impl(block_pointer b, size_t index) : m_block(b), m_index(index) {}
        reference operator*() const { return m_block->m_data[IS_REVERSE ? m_block->m_used - 1 - m_index : m_index]; }
        pointer operator->() const { return &m_block->m_data[IS_REVERSE ? m_block->m_used - 1 - m_index : m_index]; }
        ITERATOR& operator++()
        {
            if(++m_index >= m_block->m_used) 
            { 
                m_block = IS_REVERSE ? m_block->m_prev : m_block->m_next; 
                m_index = 0; 
            }
            return static_cast<ITERATOR&>(*this);
        }
        ITERATOR operator++(int)
        {
            ITERATOR previous = static_cast<ITERATOR&>(*this);
            ++(*this);
            return previous;
        }
        bool operator==(const ITERATOR& other) const
        {
            if(m_block == other.m_block)
            {
                return !m_block || other.m_index == m_index;
            }
            // const bool this_end = m_block && !m_block->m_next && m_index == m_block->m_used;
            // const bool other_end = other.m_block && !other.m_block->m_next && other.m_index == other.m_block->m_used;
            // return (this_end && !other.m_block) || (other_end && !m_block);
            return false;
        }
        bool operator!=(const ITERATOR& other) const 
        { 
            return !(*this == other); 
        }
        bool operator < (const ITERATOR & other) const
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
        bool operator > (const ITERATOR & other) const
        {
            return other < static_cast<const ITERATOR&>(*this);
        }

    private:
        friend class block_vector;
        block_pointer m_block;
        size_t m_index;
    };

    class iterator : public iterator_impl<iterator, false>
    {
    public:
        using iterator_impl<iterator, false>::iterator_impl;
    };

    class const_iterator : public iterator_impl<const_iterator, true>
    {
    public:
        using iterator_impl<const_iterator, true>::iterator_impl;
    };

    class reverse_iterator : public iterator_impl<reverse_iterator, false, true>
    {
    public:
        using iterator_impl<reverse_iterator, false, true>::iterator_impl;
    };

    class const_reverse_iterator : public iterator_impl<const_reverse_iterator, true, true>
    {
    public:
        using iterator_impl<const_reverse_iterator, true, true>::iterator_impl;
    };

    const_iterator cbegin() const;
    const_iterator cend() const;
    const_iterator begin() const;
    const_iterator end() const;
    iterator begin();
    iterator end();
    reverse_iterator rbegin();
    reverse_iterator rend();
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;
    const_reverse_iterator crbegin() const;
    const_reverse_iterator crend() const;
    const_reverse_iterator rcbegin() const;
    const_reverse_iterator rcend() const;

    ~block_vector();

    void clear();

    size_t size() const;
    bool empty() const;

    // insert a value as the last value
    void push_back(const T& value);

    // return a reference to the last value
    T & back()
    {
        assert(m_last != nullptr);
        return m_last->m_data[m_last->m_used - 1];  
    }
    const T & back() const
    {
        assert(m_last != nullptr);
        return m_last->m_data[m_last->m_used - 1];  
    }

    // todo: remove the erase and replace_two functions, if they are really not used
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
typename block_vector<T, block_size>::const_iterator block_vector<T, block_size>::cbegin() const
{
    return const_iterator(m_first, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_iterator block_vector<T, block_size>::cend() const
{
    return const_iterator(nullptr, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_iterator block_vector<T, block_size>::begin() const
{
    return const_iterator(m_first, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_iterator block_vector<T, block_size>::end() const
{
    return const_iterator(nullptr, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::reverse_iterator block_vector<T, block_size>::rbegin()
{
    return reverse_iterator(m_last, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::reverse_iterator block_vector<T, block_size>::rend()
{
    return reverse_iterator(nullptr, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_reverse_iterator block_vector<T, block_size>::rbegin() const
{
    return const_reverse_iterator(m_last, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_reverse_iterator block_vector<T, block_size>::rend() const
{
    return const_reverse_iterator(nullptr, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_reverse_iterator block_vector<T, block_size>::crbegin() const
{
    return const_reverse_iterator(m_last, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_reverse_iterator block_vector<T, block_size>::crend() const
{
    return const_reverse_iterator(nullptr, 0);
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_reverse_iterator block_vector<T, block_size>::rcbegin() const
{
    return crbegin();
}

template<typename T, size_t block_size>
typename block_vector<T, block_size>::const_reverse_iterator block_vector<T, block_size>::rcend() const
{
    return crend();
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
        m_last->m_order = m_last->m_prev->m_order + 1;
    }
    m_last->m_data[m_last->m_used++] = value;
    ++m_used;
}
